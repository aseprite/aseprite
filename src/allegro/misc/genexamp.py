#!/usr/bin/env python
# -*- mode:Python; tab-width: 3 -*-
"""
This is a helper script which reads the .c files of the examples
directory, extracts the comments from the source, and updates
allegro._tx's appropriate section with the comments after
hyperlinking functions and examples. It is not meant to be run by
the end user, only by the maintainer packaging Allegro.

To run the script go to the Allegro root dir and type "python
misc/genexamp.py". If everything goes ok, you will end up with a
patch in stdout you can apply manually after having verified it is
ok. Usage example:

   python misc/genexamp.py | less
   ...review...
   python misc/genexamp.py | patch -p0

In order to work, this script requires Python (tested with 2.3.3) and
the diff binary in your path. Written by Grzegorz Adam Hankiewicz,
gradha@users.sourceforge.net. Notify me of any broken behaviour,
like uncaught exceptions. This script falls under Allegro's giftware
license.
"""
import glob
import os
import os.path
import popen2
import re
import string
import sys

path_to_documentation = apply(os.path.join, ["docs", "src", "allegro._tx"])
path_to_example_dir = apply(os.path.join, ["examples"])
# number of max erefs, below 1 desactivates limit
limit_example_erefs = 10
limit_example_reference = "Available Allegro examples"

# text tags used to detect where the examples should be put
START_MARK = "start genexamp.py chunk"
END_MARK = "end genexamp.py chunk"
XREF_WIDTH = 70
regular_expression_for_tx_identifiers = r"@[@\\][^@]+@(?P<name>\w+)"
comment_line = re.compile(r"Example pr|Modified by")

# the order of the examples is aimed at the newbie, going from easy
# to difficult
examples_order = ["exhello", "exmem", "expal", "expat", "exflame",
   "exdbuf", "exflip", "exfixed", "exfont", "exmouse", "extimer", "exkeys",
   "exjoy", "exsample", "exmidi", "exgui", "excustom", "exunicod",
   "exbitmap", "exscale", "exconfig", "exdata", "exsprite", "exrotscl",
   "exexedat",
   "extrans", "extrans2", "extruec", "excolmap", "exrgbhsv", "exshade",
   "exblend", "exxfade", "exalpha", "exlights", "ex3d", "excamera", "exquat",
   "exstars", "exscn3d", "exzbuf", "exscroll", "ex3buf", "ex12bit",
   "exaccel", "exspline", "exsyscur", "exupdate", "exswitch",
   "exstream", "expackf"]

# Holds examples' short descriptions. Loaded from disk at runtime.
short_descriptions = {}


def detect_all_available_examples(path):
   """func(path_to_directory)

   Checks if the maintainer is doing his work and didn't miss
   some example in the global list, printing any missing examples
   to stderr.
   """
   examples = filter(lambda x: x not in examples_order,
      map(lambda x: os.path.splitext(os.path.basename(x))[0],
      glob.glob(os.path.join(path, "*.c"))))
   if examples:
      sys.stderr.write("Warning! There are examples not listed in the default\n"
         "examples_order variable. You should correct this:\n")
      for example in examples:
         sys.stderr.write("\t%s\n" % example)
      examples_order.extend(examples)
      


def load_example_short_descriptions(path):
   """func(path_to_directory)

   Loads the file examples.txt from the specified directory and
   extracts the short example descriptions which are in "exblah -
   text" format. The short descriptions are stored in the global
   dictionary short_descriptions.
   """
   file_input = file(os.path.join(path, "examples.txt"), "rt")
   try:
      regex = re.compile(r"(ex\w+)[.]c\s+-\s(.*)")
      line = file_input.readline()
      while line:
         m = regex.match(line)
         if m:
            short_descriptions[m.group(1)] = m.group(2)
            
         line = file_input.readline()
   finally:
      file_input.close()

   

def retrieve_documentation_identifiers(file_name):
   """func(path_to_documentation._tx) -> {documentation_identifiers}

   Used on a text file with the format of Allegro's documentation,
   extracts all the valid identifiers and returns them as the keys
   of a dictionary, whose values are all 1 and can be ignored.
   """
   dic = {}
   exp = re.compile(regular_expression_for_tx_identifiers)
   file = open(file_name, "rt")
   for line in file.readlines():
      match = exp.match(line)
      # avoid the identifiers of existant examples
      if match and match.group("name") not in examples_order:
         dic[match.group("name")] = 1
   file.close()
   return dic

   

def retrieve_source_data(file_name):
   """func(path_to_example.c) -> ([comment_lines], {detected_identifiers})

   Used on the source code of an example, extracts the first comment
   after deleting the initial lines which credit authors. The comment
   will be used as the text for the documentation. Also returns a
   dictionary with all the keys being hypotetical identifiers used
   in the code, that is, every single word susceptible of being
   used as an allegro function, like return, printf, allegro_init,
   SCREEN_W, etc.
   """
   dic = {}
   exp = re.compile(r"\w+")
   file = open(file_name, "rt")
   getting_comment = 1
   comment = []
   for line in file.readlines():
      if getting_comment:
         # stop reading after first comment end marker
         if string.find(line, "*/") >= 0:
            getting_comment = 0
         else:
            # ok, retrieve all lines removing first 3 characters
            comment.append(string.strip(line[2:]))
      else:
         # retrieve all possible keywords
         for match in re.findall(exp, line):
            dic[match] = 1
   file.close()

   # post-process comment, removing initial credits
   while len(comment):
      if string.strip(comment[0]) == "":
         comment.pop(0)
      elif comment_line.search(comment[0]):
         while len(comment) and string.strip(comment[0]) != "":
            comment.pop(0)
      else:
         break
   return comment, dic

   

def get_valid_example_ids(example_ids, global_ids, incremental = None):
   """func({example_ids}, {global_ids}, bool) -> [valid_identifiers]

   Compares the dictionary of a source example with all its possible
   identifiers against the dictionary with all the available
   documentation identifiers and returns a filtered list of the
   example identifiers which are also in the global dictionary.

   If the incremental parameter is true, the identifiers of
   global_ids that are also contained in example_ids will be
   erased. This is useful if you are building example after example
   and you don't want new lists to include the identifiers returned
   in previous calls to this function.
   """
   example_ids = example_ids.keys()
   example_ids.sort()
   valid_ids = []
   if incremental:
      for id in example_ids:
         try:
            del global_ids[id]
            valid_ids.append(id)
         except KeyError:
            pass
   else:
      for id in example_ids:
         if global_ids.has_key(id):
            valid_ids.append(id)
   return valid_ids



def build_formatted_lines(word_list):
   """func([word1, word2, ...]) -> [line1, line2, ...]

   Consumes the given list of words and returns a list of text
   lines. Each line will contain several words joined with the string
   ", ". All lines will have a width <= XREF_WIDTH.
   """
   new_list = []
   new_line = []
   length = 0
   while len(word_list) > 0:
      new_word = word_list.pop(0)
      if length + len(new_word) + 2 < XREF_WIDTH:
         length = length + len(new_word) + 2
         new_line.append(new_word)
      else:
         new_list.append(string.join(new_line, ", "))
         new_line = [new_word]
         length = len(new_word) + 2

   new_list.append(string.join(new_line, ", "))
   return new_list

      

def build_xref_block(xref_list, xref_tag = None):
   """func([word1, word2, ...], string) -> [line1, line2, ...]

   Behaves like build_formatted_lines, adding to the beginning of
   each returned line the string given as second parameter. If the
   string is None, "@xref" will be prepended.
   """
   lines = []
   if not xref_tag: xref_tag = "@xref"
   for line in build_formatted_lines(xref_list):
      lines.append("%s %s\n" % (xref_tag, line))
   return lines


   
def build_example_output(example_name, comment_lines, xref_list):
   """func(example_name, [comment], [xrefs]) -> [formatted_lines]

   Given an example name (the part of the file_name without
   extension), its source commentary, and the references it makes,
   returns a list of text lines formatted like this:

   @@Example @example_name
   @@xrefs
   @shortdesc
      comment
   """
   lines = ["@@Example @%s\n" % example_name]
   lines.extend(build_xref_block(xref_list))
   if short_descriptions.has_key(example_name):
      lines.append("@shortdesc %s\n" % short_descriptions[example_name])
   lines.extend(map(lambda x: "   %s\n" % x, comment_lines))
   lines.append("\n")
   return lines


def generate_example_chapter(path_to_documentation, example_files, incremental = None):
   """func(path._tx, [paths_to_examples], bool) -> ([doc], {ids_to_examples})

   First retrieves fom path_to_documentation the valid identifiers.
   Then goes through the list of paths to examples and extracts
   from each of them the used identifiers. Returns a list of lines
   which contains the new generated documentation. Also returns a
   dictionary whose keys are identifiers found in the documentation
   file, with the values being a list of the examples which refer
   to the key.

   If incremental is false, each documented example will reference
   all the possible identifiers found in the documentation. But if
   it's true, each example will create references only to identifiers
   which still have not been referenced before.
   """
   reverse_docs = {}
   doc_ids = retrieve_documentation_identifiers(path_to_documentation)
   lines = []
   for example in example_files:
      comment, ex_ids = retrieve_source_data(example)
      name = os.path.splitext(os.path.basename(example))[0]
      valid_ids = get_valid_example_ids(ex_ids, doc_ids, incremental)
      # build reverse lookup
      for id in valid_ids:
         try:
            reverse_docs[id].append(name)
         except KeyError:
            reverse_docs[id] = [name]
      # finally add the text chunk
      lines.extend(build_example_output(name, comment, valid_ids))
   return lines, reverse_docs

   

def replace_example_chapter(path_to_documentation, chapter_lines):
   """func(path_to_doc._tx, [new_chapter]) -> [regenerated_documentation]

   Opens the documentation and searches for the text section
   separated through the global marks START_MARK/END_MARK. Returns
   the opened file with that section replaced by chapter_lines.
   """
   lines = []
   file = open(path_to_documentation, "rt")
   state = 0
   for line in file.readlines():
      if state == 0:
         lines.append(line)
         if string.find(line, START_MARK) > 0:
            state = state + 1
      elif state == 1:
         if string.find(line, END_MARK) > 0:
            lines.extend(chapter_lines)
            lines.append(line)
            state = state + 1
      else:
         lines.append(line)

   return lines

   

def find_differences(file_name, lines):
   """func(path_to_doc._tx, [text_lines]) -> [diff_lines]

   This function runs diff over the given document file and text
   lines.  To make the comparison a temporary file with extension
   .tmp is build and later removed. The function returns the output
   of the diff commant as a list of lines.
   """
   temp_name = string.replace(path_to_documentation, "._tx", ".tmp")
   file = open(temp_name, "wt")
   file.writelines(lines)
   file.close()

   # call external tool and read it's stdout
   stdout, stdin = popen2.popen2 (["diff", "-u", file_name, temp_name])
   stdin.close()
   diff_lines = stdout.readlines()
   stdout.close()
   os.unlink(temp_name)
   return diff_lines


   
def replace_example_references(documentation, ids_to_examples):
   """func([lines], {id: [words]}) -> [new_lines]

   Goes through the documentation in memory searching for the
   identifiers.  When one is found, it looks it up in the provided
   dictionary. If it's not found, nothing happens. Otherwise whatches
   closely the following text lines for @erefs and updates them
   accordingly. If no @erefs are found, they are created. Finally
   a new list is returned with the updated documentation.

   There is a special hardcoded case: if limit_example_erefs
   is greater than 0, this value will be used as the limit of
   example references generated per identifier. When the number
   of example references is greater than this limit, a single
   reference is generated to the value stored in the string
   limit_example_reference, which should point to the correct
   Allegro documentation chapter.
   """
   new_lines = []
   short_desc = []
   found_id = ""
   exp = re.compile(regular_expression_for_tx_identifiers)
   for line in documentation:
      if found_id:
         if line[0] == '@':
            # Don't append erefs, which will be regenerated.
            if line[1:5] == "xref" or line[1] == "@" or line[1:3] == "\\ ":
               new_lines.append(line)
            # Append shortdesc, but after @erefs as a special case.
            if line[1:10] == "shortdesc":
               short_desc = [line]
         else:
            # create the eref block and append before normal text
            eref_list = ids_to_examples[found_id]
            eref_list.sort()
            if len(eref_list) > limit_example_erefs:
               new_lines.append("@eref %s\n" % limit_example_reference)
            else:
               new_lines.extend(build_xref_block(eref_list, "@eref"))
               
            if short_desc:
               new_lines.extend(short_desc)
               short_desc = []
               
            new_lines.append(line)
            found_id = ""
      else:
         if short_desc:
            new_lines.extend(short_desc)
            short_desc = []
            
         new_lines.append(line)
         match = exp.match(line)
         if match and ids_to_examples.has_key(match.group("name")):
            found_id = match.group("name")
            
   return new_lines

   

def main(path_to_documentation, path_to_example_dir, incremental):        
   detect_all_available_examples(path_to_example_dir)
   load_example_short_descriptions(path_to_example_dir)
   chapter, ids_to_examples = generate_example_chapter(path_to_documentation,
      map(lambda x: os.path.join(path_to_example_dir, "%s.c" % x),
      examples_order), incremental)

   documentation = replace_example_chapter(path_to_documentation, chapter)
   documentation = replace_example_references(documentation, ids_to_examples)

   differences = find_differences(path_to_documentation, documentation)
   sys.stdout.writelines(differences)



if __name__ == "__main__":
   main(path_to_documentation, path_to_example_dir, None)
