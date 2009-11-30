# Splits the rst .html output in various .html files
# Copyright (C) 2009 David Capello

from xml.dom import Node
from xml.dom.minidom import parse, parseString

def getElementById(node, id):
    if node.hasAttribute("id") and node.getAttribute("id") == id:
        return node
    for child in node.childNodes:
        if child.nodeType == Node.ELEMENT_NODE:
            found = getElementById(child, id)
            if found:
                return found
    return None

def remapAHref(node, old, new):
    if node.tagName == "a" and node.hasAttribute("href") and node.getAttribute("href") == old:
        node.setAttribute("href", new)
    for child in node.childNodes:
        if child.nodeType == Node.ELEMENT_NODE:
            remapAHref(child, old, new)

def collectIdsInSection(node, ids, section):
    if section:
        if node.hasAttribute("id"):
            ids.append([ section, node.getAttribute("id") ])
    elif node.tagName == "div" and \
            ( node.hasAttribute("class") and node.getAttribute("class") == "section") or \
            ( node.hasAttribute("id") and node.getAttribute("id") == "contents"):
        if node.getAttribute("id") == "contents":
            section = "index"
        else:
            section = node.getAttribute("id")

    for child in node.childNodes:
        if child.nodeType == Node.ELEMENT_NODE:
            collectIdsInSection(child, ids, section)
    
def splitSection(doc, sectionId, removePrevs, outputFileName, idsToRemap):
    # clone the whole document
    doc2 = doc.cloneNode(True)

    # get the <div> element containing the specified section
    div = getElementById(doc2.documentElement, sectionId)
    if not div:
        return

    # remove previous elements (<div> of previous sections and content table)
    if removePrevs:
        while div.previousSibling:
            div.parentNode.removeChild(div.previousSibling)

    # remove next elements (<div> of next sections)
    while div.nextSibling:
        div.parentNode.removeChild(div.nextSibling)

    file = open(outputFileName, "w")
    doc2.writexml(file)
    file.close()

###################################################################### 

manualDoc = parse("manual.html")

# get IDs from any tag with a id="..." attribute
idsToRemap = []
collectIdsInSection(manualDoc.documentElement, idsToRemap, None)

# sections
sectionsIds = []
for id in idsToRemap:
    sectionsIds.append(id[0])
sectionsIds = set(sectionsIds)

# remap sections
for sectionId in sectionsIds:
    remapAHref(manualDoc.documentElement, "#" + sectionId, sectionId + ".html")

# remap all href to the future location
for id in idsToRemap:
    remapAHref(manualDoc.documentElement, "#" + id[1], id[0] + ".html#" + id[1])

# create a file for the content table
splitSection(manualDoc, "contents", False, "index.html", idsToRemap)

# create a file for each section
for sectionId in sectionsIds:
    splitSection(manualDoc, sectionId, True, sectionId + ".html", idsToRemap)
