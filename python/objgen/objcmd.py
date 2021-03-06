#!/usr/bin/env python
import os, sys, re
from copy import copy, deepcopy

class Commands:
  '''
  commands imbed in the comments
  s doesn't include /* */ or //
  after initialization:
    cmd = Commands()
  you can always use cmd["anykey"], even if "anykey" is not present,
  the behavior differs from python dict, which raises a KeyError
  '''
  def __init__(self, s):
    # do symbol substitution first
    self.cmds = {}
    self.persist = {}
    # shouldn't subst symbols before parse commands
    self.raw = s;  # self.subst_symbols(s)
    self.parse_commands()

  # make the object looks like a dictionary
  def __getitem__(self, key):
    return self.cmds[key] if key in self.cmds else None
  def __setitem__(self, key, value):
    self.cmds[key] = value
  def __delitem__(self, key):
    if key == None: return
    del self.cmds[key]
  def __iter__(self):
    return self.cmds.__iter__()
  def __contains__(self, key):
    return self.cmds.has_key(key)
  def __len__(self):
    return len(self.cmds)
  def __str__(self):
    return str(self.cmds)
  def __repr__(self):
    return repr(self.cmds)
  def __copy__(self): # awkward!
    c = Commands("")
    c.raw = s.raw
    c.cmds = copy(self.cmds)
    c.persist = copy(self.persist)
    return c
  def __deepcopy__(s, memo):
    c = Commands("")
    memo[id(s)] = c
    for n, v in s.__dict__.iteritems():
      setattr(c, n, deepcopy(v, memo))
    return c

  def subdash(self, s):
    return re.sub(r"\-", "_", s)  # map - to _

  def parse_commands(self):
    '''
    parse raw text in comment to commands, save results to .cmds
    cmds['desc'] being the description
    the process
    '''
    s = self.raw
    if len(s) == 0:
      self.cmds["desc"] = "" # add an empty description
      return
    pos = 0

    '''
    for multiple-line comments
    remove the leading`* ' for subsequent lines
    '''
    sa = [a.strip() for a in s.splitlines()]
    if len(sa) > 1: # sa[:1] is the first line
      sa = sa[:1] + [(a[2:].rstrip() if a.startswith("* ") else a) for a in sa[1:] ]
    s = '\n'.join(sa)
    while 1:
      '''
      command line:
        $cmd op args;
      op can be one of ":", "=", ":=", "::" or "" (nothing)
        : or = means set the current variable only
        := or :: means a persistent command that apply to
                 all following items
                 if args is empty, the command is removed
      '''
      goodcmd = 1
      pattern = r"[^\$\\]*(\$)([\w\-]+)\s*([\+\-\:]?=|:?:)\s*(.*?)((?<!\\)\;|$)"
      m = re.search(pattern, s, re.MULTILINE | re.DOTALL) # full command
      if m:
        '''
        group 2 is the cmd
        group 3 is the operator
        group 4 is the argument
        '''
        cmd  = self.subdash(m.group(2))
        args = m.group(4)
        if m.group(3) in (":=", "::"):
          # we assume @0 means $0, it's just too confusing otherwise
          if args.strip() in ("$0", "@0"): # means removing a command from the persistent list
            #print "turning off an persistent command, raw=[%s]" % s; raw_input()
            self.persist[cmd] = -1
            goodcmd = 0
          else:
            #print "set an persistent command, raw=[%s] args=[%s]" % (s, args); raw_input()
            self.persist[cmd] = 1
        #print "multiple %s]\ns=%s\nm0=%s\nself.raw=%s" % (args, s, m.group(0),repr(self.raw))
        #raw_input()
      else:
        if not "$" in s: break
        # print "look for a lazy command, $cmd with no ; s = [%s]" % s
        pattern = r"[^\$\\]*(\$)(\w+)\;?"
        m = re.search(pattern, s)
        if m:
          # print "found a lazy command, $cmd with no ; s = %s" % s
          cmd = self.subdash(m.group(2))
          args = 1  # means turn on the switch
        else:
          # search for comment command
          pattern = r"[^\$\\]*(\$#)(.*?)([^\\]\;|$)"
          m = re.search(pattern, s)
          if m == None:
            print "possibly an unknown command [%s]" % s
            break
          goodcmd = 0
          #print "a comment is found [%s]" % m.group(0)

      if goodcmd:
        self.cmds[cmd] = self.subst_symbols(args) # add to dictionary
      s = s[:m.start(1)] + s[m.end(0):] # remove the command from comment
      #print "a pattern is found [%s]: [%s], rest: %s" % (
      #    cmd, param, s)
      #raw_input()

    # if desc is not explicitly specified
    # we join all the remaining non-command contents
    # left in the comment
    if not "desc" in self.cmds:
      sa = [a.strip() for a in s.splitlines()] # split to lines
      s = ' '.join(sa).strip().rstrip(",;.")
      self.cmds["desc"] = s

  def subst_symbols(self, s):
    '''
    $$ or \$  =>  $
    \;        =>  ;
    '''
    if s in (None, 1): return s
    if len(s) == 0: return ""
    # merge the remaining string to description
    s = re.sub(r"[\$\\]\$", "$", s) # literal $
    s = re.sub(r"\\;", ";", s) # literal ;
    s = re.sub(r"\\n", "\n", s) # literal ;
    return s

  def addpre(self, pif, pelse, pendif):
    ''' preprocessor commands '''
    cmd = "#if"

    if pif:
      self.cmds[cmd] = pif
      self.persist[cmd] = 1

    if pelse:
      self.persist[cmd] = -2

    if pendif:
      assert (not pelse and not pif)
      self.persist[cmd] = -1

