#!/usr/bin/env python
import os, sys, re, filecmp
from copy import copy

'''
intend to be a code generator
TODO:
  * multiple-line support with regular expression
  * preprocessor
  * interpretor
  * C++ comment
  * skip struct in comment or string
'''


cmt0 = "/*"
cmt1 = "*/"
lncmt = "//"

class P:
  '''
  simple position in the code (line, column)
  unfortunately, the most vulnerable class
  '''
  def __init__(self, row = 0, col = 0):
    self.row = row
    self.col = col
    self.check() # see if input are integers
  def __str__(self):
    self.check()
    return "line: %d, col %d" % (self.row + 1, self.col + 1)
  def __repr__(self):
    self.check();
    return "%d, %d" % (self.row, self.col)
  def __cmp__(a, b):
    a.check()
    if type(a) != type(b):
      print "trying to compare %s with %s" % (type(a), type(b))
      raise Exception
    b.check()
    return (a.row - b.row) * 10000 + a.col - b.col
  def check(self):
    if type(self.row) != int:
      print "row %s is not a number" % (self.row)
      raise Exception
    if type(self.col) != int:
      print "col %s is not a number" % (self.col)
      raise Exception
  def getline(p, src):
    ''' get a line starting from the current position '''
    return src[p.row][p.col:] if p.row < len(src) else None
  def nextline(self):
    self.check()
    self.row += 1
    self.col = 0

  def skipspace(p, src):
    '''
    skip spaces (including multiple blank lines)
    return the line
    '''
    p.check()
    while 1:
      s = p.getline(src)
      if s == None:
        self.ttype = self.token = None
        break
      # skip leading space
      m = re.match(r"(\s*).*", s)
      if not m: raise Exception
      p.col += m.end(1)
      s = p.getline(src)
      if len(s) != 0: break # found a token
      # this line is exhausted
      p.nextline()
      continue
    return s

  def gettoken(p, src):
    '''
    aggressively get a token starting from the current `p'
    `p' is updated to the end of the current token
    return the token only
    '''
    s = p.skipspace(src)
    
    # try to get a word
    if s != None:
      m = re.match(r"([a-zA-Z_]\w*).*", s)
      if m:
        p.col += m.end(1)
        p.ttype, p.token = "word", s[:m.end(1)]
      else:
        p.col += 1
        p.ttype = p.token = s[0]
    return p.token, p.ttype

  def ungettok(p, src):
    '''
    try to unget a token, it will not unget space
    '''
    p.check()
    tok = p.token
    if not tok:
      print "cannot unget None"
      raise Exception
    p.col -= len(tok)
    if (p.col < 0 or
        not p.getline(src).startswith(tok)):
      raise Exception
    p.token = p.ttype  = None

  def peektok(p, src):
    '''
    peek at the next token
    '''
    p.check()
    q = copy(p) # create a backup
    return q.gettoken(src)

  def gettext(self, src, q):
    '''
    get entire string from p to q
    '''
    p = copy(self) # so self is intact
    if q <= P(0,0):
      q = P(len(src), 0) # end of string
    s = ""
    while p.row < q.row:
      s += p.getline(src)
      p.nextline()
    if p.row == q.row and p.col < q.col:
      s += src[p.row][p.col: q.col]
    return s

class Decl:
  '''
  C declaration, adapted from K & R
  '''
  def __init__(self, src, p):
    self.empty = 1
    self.parse(src, p)

  def parse(self, src, p):
    p.skipspace(src)
    self.begin = copy(p)

    # pre-filter out the case where the first token is not a word
    token, type = p.peektok(src)
    if type != "word": return -1

    self.param_level = 0
    self.types = []
    # find the type, e.g. int, void, ...
    if 0 != self.dclspec(src, p): return -1
    # find the declarator, e.g. *a, (*a)(), ...
    if 0 != self.dcl(src, p): return -1
    self.types += [self.datatype]

    # handle the last semicolon
    token, ttype = p.gettoken(src)
    if token != ";":
      print "missing ; %s" % self.dbg(src, p)
      return -1
    
    self.end = copy(p)
    self.raw = self.begin.gettext(src, self.end)
    self.empty = 0
    #print "complete a decl {%s} from %s to %s\n{%s}" % (self, self.begin, self.end, self.raw)
    return 0

  def dclspec(self, src, p, allow_empty = 0):
    # storage classifiers: auto, register, static, extern, typedef
    # type qualifier: const, volatile
    nwords = 0
    while 1:
      token, ttype = p.gettoken(src)
      if ttype != "word":
        if allow_empty and nwords == 0:
          p.ungettok(src)
          break
        print "expected data type, %s" % (self.dbg(src, p))
        raise Exception
        return 1
      nwords += 1
      if not token in ("auto", "register", "static", "extern"
          "const", "volitale"):
        if self.param_level == 0:
          self.datatype = token
          #print "datatype is [%s], pos %s" % (token, p)
          #raw_input()
        break
    return 0

  def dcl(self, src, p):
    '''
    declarator
    wrapper: counter the leading # of stars, and lead to dirdcl()
    '''
    ns = 0
    while 1:
      token, type = p.gettoken(src)
      if token != "*":
        p.ungettok(src)
        break
      ns += 1
    if self.dirdcl(src, p) != 0: return -1
    if self.param_level == 0: self.types += ["pointer"] * ns
    return 0

  def dirdcl(self, src, p):
    '''
    direct declarator
    '''
    token, ttype = p.gettoken(src)
    if ttype == '(': # ( dcl )
      self.dcl(src, p) 
      if p.gettoken(src) != (')', ')'):
        print "missing ) %s" % (self.dbg(src, p))
        raise Exception
    elif ttype == "word":
      self.name = token
    elif self.param_level > 0:
      p.ungettok(src)
    else:
      print "expected name or (, %s" % (self.dbg(src, p))
      raise Exception
    while 1:
      token, ttype = p.gettoken(src)
      if ttype == "(":
        self.ptlist(src, p)
        token, type = p.gettoken(src)
        if token != ')':
          print "missing ) after parameter list, token: [%s], %s" % (token, self.dbg(src, p))
          raise Exception
        if self.param_level == 0: self.types += ["function"]
      elif ttype == "[": 
        # array dimension can only span a one line
        pattern = r"(.*)\]"
        s = p.getline(src)
        m = re.match(pattern, s)
        if m:
          token = s[m.start(1) : m.end(1)]
          if self.param_level == 0: self.types += ["array" + token]
          p.col += m.end(0)
        else:
          print "missing ], %s" % (self.dbg(src, p))
          raise Exception
      else:
        p.ungettok(src)
        break
    return 0

  def ptlist(self, src, p):
    ''' parameter type list '''
    while 1:
      self.pdecl(src, p)
      token, ttype = p.gettoken(src)
      if token != ',':
        p.ungettok(src)
        break

  def pdecl(self, src, p):
    ''' parameter declarator '''
    self.param_level += 1
    self.dclspec(src, p)
    self.dcl(src, p)
    self.param_level -= 1

  def __str__(self):
    return ' '.join(self.types) + ' ' + self.name

  def __repr__(self):
    return self.types

  def dbg(self, src, p):
    ''' return the debug message '''
    return "line: [%s], pos: %s" % (p.getline(src).rstrip(), p)

  def isempty(self): return self.empty

class Comment:
  '''
  C block-comment or one-line comment
  '''
  def __init__(self, src, p):
    self.empty = 1
    if 0 != self.span(src, p):       # .begin, .end, and .raw
      return
    self.get_commands()     # .cmds with .cmds['desc'] being the description
    self.empty = 0

  def span(self, src, p):
    '''
    find the beginning and end of a comment,
    starting from pos
    '''
    s = p.skipspace(src) # note, its skips multiple blank lines
    if s == None: return -1

    if s.startswith(cmt0) or s.startswith(lncmt):
      self.begin = copy(p)
    else:
      print "missing comment beginning mark %s" % p
      return -1

    p.col += 2 # skip `/*' or `//'

    if s.startswith(lncmt): # line comment
      self.raw = s[2:].strip()
      p.nextline()
      self.end = copy(p)
    else: # block comment
      # aggressively search the end of comment
      while 1:
        s = p.skipspace(src)
        if s.startswith(cmt1):
          p.col += len(cmt1)
          self.end = copy(p)
          #print "comment found from %s to %s" % (self.begin, self.end)
          break
        p.gettoken(src) # get token by token
      else:
        print "Warning: cannot find the end of comment from %s, now = %s" % (
            self.begin, now)
        raise Exception
      self.raw = self.extract_text(src)
    return 0

  def extract_text(self, src):
    '''
    extract text within block comment
    '''
    self.checkbe()
    s = self.begin.gettext(src, self.end).strip()
    if s.startswith(cmt0) and s.endswith(cmt1):
      s = s[len(cmt0) : len(s) - len(cmt1)]
    #print "comment text is [%s] from %s to %s" % (s, self.begin, self.end)
    return s

  def checkbe(self):
    self.begin.check()
    self.end.check()

  def get_commands(self):
    '''
    parse text to commands, save results to .cmds
    '''
    self.cmds = {}
    s = self.raw
    pos = 0
    while 1:
      # note: $$ or \$ means a literal $
      # : or = means set the current
      # := or :: means set the parser's property
      pattern = r"[^\$\\](\$)(\w+)\s*([\:\=]{1,2})\s*(.*?)(\;)"
      m = re.search(pattern, s, re.MULTILINE | re.DOTALL)
      if m == None: break
      # a command is found
      cmd = s[m.start(2) : m.end(2)]
      persist = m.end(3) - m.start(3) - 1 # the operator
      param = s[m.start(4) : m.end(4)]
      self.cmds[cmd] = [param, persist] # add to dictionary
      s = s[:m.start(1)] + s[m.end(5):]; # remove the command
      #print "a pattern is found [%s]: [%s], rest: %s" % (
      #    cmd, param, s)
      #raw_input()

    # merge the remaining string to description
    if not self.cmds.has_key("desc"):
      s = re.sub(r"[\$\\]\$", "$", s)
      sa = [a.strip() for a in s.splitlines()] # split to lines
      s = ' '.join(sa).strip().rstrip(";,.")
      self.cmds["desc"] = [s, 0] # description is not persistent
      #print "the remain string is [%s]" % self.cmds["desc"]

  def isempty(self): return self.empty
  def __str__(self): return self.raw
  def __repr__(self): return __str__(self)


class Item:
  '''
  item:
    preprocessor
    declaration
    declaration comment
    comment
  '''

  def __init__(self, src, p):
    '''
    initialize the structure from a piece of code
    '''
    self.empty = 1
    self.begin = copy(p)
    self.parse(src, p)

  def parse(self, src, p):
    '''
    parse an item aggressively
    '''
    token,ttype = p.peektok(src)

    if token == "}": return -1
  
    # try to get a declaration
    decl = Decl(src, p)
    self.decl = decl if not decl.isempty() else None
    
    # try to get a comment
    comment = Comment(src, p)
    self.comment = comment if not comment.isempty() else None
    
    if self.comment or self.decl:
      #print "successfully got one item, at %s" % p
      self.empty = 0
    #self.get_generic_type()
    
  def find_type(self):
    '''
    parse the declaration string, to obtain the name and type
    '''
    s = self.decl
   
    chmap = {} # use dictionary

    # check the validity of the declaration
    unwanted = "~`!@#$%^&-+={}|\\:;'\"<>,.?/"
    for c in s:
      if c in unwanted:
        print "bad declaration [%s], has character [%s]" % (s, c)
        raise Exception
      else:
        chmap[c] = (1 if not chmap.has_key(c) else (chmap[c]+1))

    d = Decl(s)
    self.types = d.types
    self.name = d.name
    #print "type stack:", self.types

  def get_generic_type(self):
    '''
    get a generic type, and type name
    '''
    types = self.types
    nlevels = len(types)
    if nlevels == 1:
      self.typeg = ("simple", types[0])
    elif types[0].startswith("array"):
      self.typeg = ("static array", ' '.join(types[1:]))
      self.arrcnt = types[0][5:]
    elif types[0] == "pointer":
      towhat = ' '.join(types[1:])
      if nlevels == 2 and types[1] == "function":
        self.typeg = ("function", ' '.join(types[2:]))
      elif 'arr' in self.cmds:
        if 'obj' in self.cmds and nlevels == 2:
          self.typeg = ("object array", towhat)
        else:
          self.typeg = ("dynamic array", towhat)
      elif 'obj' in self.cmds:
        self.typeg = ("object", towhat)
      else:
        self.typeg = ("pointer", towhat)

  def isempty(self):
    return self.empty

class Object:
  '''
  a struct defined as
  typedef struct {
  } abc_t;
  '''
  def __init__(self, src, pos = None):
    ''' initialize myself from a bunch of src '''
    if len(src) == 0: return # return an empty object

    if type(pos) == int:  # integer input
      pos = P(pos, 0)
    self.items = []
    self.empty = 1
    self.parse(src, pos)

  def parse(self, src, p):
    ''' parse src starting from 'p' '''
   
    # print "start searching object from %s" % p
    if not self.find_beginning(src, p):
      return

    while 1: # parse the content within { ... },
      # aggressively search for an item, and update p
      item = Item(src, p)
      if item.isempty(): break

      #print "new item: %s" % (item)
      self.items += [item]
   
    # parse }
    self.find_ending(src, p, 0)

  def find_beginning(self, src, p, aggr = 1):
    '''
    search the block-beginning marker {
    aggressive search: repeat until found
    position 'p' is move to the end of file
      if nothing is found
    limited to single-line case
    '''
    while 1:
      line = p.getline(src)
      if line == None: break

      pattern = r"\s*(typedef)\s+struct\s*\w*\s*\{"
      m = re.search(pattern, line)
      if m: # a match is found
        self.begin = P(p.row, p.col + m.start(1))
        p.col += m.end(0)
        #print "object-beginning is found in %s, %s" % (p, src[p.row])
        return 1
      if aggr:
        p.nextline()
      else:
        break
    return 0  # not found
  
  def find_ending(self, src, p, aggr = 0):
    '''
    search the block-ending mark
    passive search: since we assume aggressive search has been
    performed for items, we only search the current line
    position 'p' is unchanged, if nothing is found
    '''
    while 1:
      line = p.getline(src)
      if line.strip() == "": # blank line 
        p.nextline()
        continue
      pattern = r'\s*(\}\s*)(\w+)(\s*;).*'
      m = re.match(pattern, line)
      if m:
        p.col += m.end(3)
        self.name = line[m.start(2) : m.end(2)]
        self.end = copy(p)
        self.empty = 0
        #print "object-ending is found in %s, %s" % (p, src[p.row])
        return 1
      if aggr:
        p.nextline()
      else:
        break
    print "no object-ending is found, p at", p
    return 0  # not found

  def isempty(self):
    return self.empty

  def output_decl(self):
    '''
    return a string for formatted declaration
    '''
    s = "typedef struct {\n"
    block = [] # block list
    for item in self.items:
      decl0 = decl1 = cmt = ""
      # 1. handle declaration
      if item.decl:
        decl0 = item.decl.datatype
        raw   = item.decl.raw
        decl1 = raw[len(decl0) : ].strip()
        #raw_input ("%s %s" % (decl0, decl1))
      # 2. handle comment
      comment = item.comment
      if (comment and comment.cmds.has_key("desc")
          and len(comment.cmds["desc"][0]) > 0):
        cmt = "/* " + comment.cmds["desc"][0] + " */"
      #print "%s %s %s, block size: %d" % (decl0, decl1, cmt, len(block))
      # 3. output the content
      if len(decl0) > 0:  # put it to a code block
        block += [(decl0, decl1, cmt)] # just buffer it
      else:
        s = self.get_decl_dump(block, s)
        block = [] # empty the block
        # also print this line
        if len(cmt) > 0: 
          s += self.get_indent(s) + cmt + "\n"
    s = self.get_decl_dump(block, s)
    block = []
    s +=  "} " + self.name + ";"
    return s
  def get_decl_dump(self, block, s):
    if len(block) == 0: return s
    # align and format all items in the bufferred block
    # and append the result to string s
    wid0 = max(len(item[0]) for item in block)
    wid1 = max(len(item[1]) for item in block)
    for item in block:
      line = "%-*s %-*s %s\n" % (wid0, item[0], wid1, item[1], item[2])
      s += self.get_indent(s) + line
    return s
  def get_indent(self, s):
    return "  " if s.endswith("\n") else ""

class Parser:
  '''
  handle objects in a file
  '''
  def __init__(self, src):
    self.src = src
    self.parse_lines()

  def parse_lines(self):
    '''
    process the input template 
    '''
    objs = self.objs = []
    p = P()

    # search for objects
    while 1:
      obj = Object(self.src, p) # try to get an object
      if obj.isempty(): break

      print "found a new object '%s' with %d items, from %s to %s\n" % (
          obj.name, len(obj.items), obj.begin, obj.end)
      objs += [obj]
    print "I got %d objects" % (len(objs))

  def output(self):
    '''
    return the completed C source code
    '''
    objs = self.objs
    src = self.src
    p0 = P(0,0)
    s = p0.gettext(src, objs[0].begin)
    nobjs = len(objs)
    for i in range(nobjs):
      obj  = objs[i]
      s += obj.output_decl()
      if i == nobjs - 1:
        pnext = P(len(src), 0)
      else:
        pnext = objs[i+1].begin
      s += obj.end.gettext(src, pnext)

    return s
   

def handle(file, template = ""):
  ''' generate a C file according to a template '''
  # guess the template name if it's empty
  if template == "":
    pt = os.path.splitext(file)
    template = pt[0] + '.0' + pt[1]

  # read in the template
  src = open(template, 'r').readlines()
  # construct a parser
  psr = Parser(src)
  # write the processed file
  open(file, 'w').write(psr.output())

def main():
  handle("test.c")

if __name__ == "__main__":
  main()

