'''
given the input JSON file of SegmentationCategoryTypeModifier type, output
a JSON that contains only codes that include 3dSlicerLabel attribute. While
doing this, make sure that attributes of the parent elements are preserved
when the codes for modifiers/types contain 3dSlicerLabel attribute

I am sure there is a better way to do this! -@fedorov

'''

import json, sys, csv, re

jFileIn = sys.argv[1]

jIn= json.loads(open(jFileIn).read())

# clean up Modifiers that don't have Slicer labels
for c in range(len(jIn["SegmentationCodes"]["Category"])):
  for t in range(len(jIn["SegmentationCodes"]["Category"][c]["Type"])):
    if "Modifier" in jIn["SegmentationCodes"]["Category"][c]["Type"][t].keys():
      origList = jIn["SegmentationCodes"]["Category"][c]["Type"][t]["Modifier"]
      newList = [i for i in origList if "3dSlicerLabel" in i.keys()]
      if len(newList):
        jIn["SegmentationCodes"]["Category"][c]["Type"][t]["Modifier"] = newList
      else:
        jIn["SegmentationCodes"]["Category"][c]["Type"][t].pop("Modifier")

# clean up types that don't have modifiers and don't have Slicer label
for c in range(len(jIn["SegmentationCodes"]["Category"])):
  origList = jIn["SegmentationCodes"]["Category"][c]["Type"]
  newList = [i for i in origList if (("3dSlicerLabel" in i.keys()) or ("Modifier" in i.keys()))]
  if len(newList):
    jIn["SegmentationCodes"]["Category"][c]["Type"] = newList
  else:
    jIn["SegmentationCodes"]["Category"][c].pop("Type")

# clean up empty categories
origList = jIn["SegmentationCodes"]["Category"]
newList = [i for i in origList if "Type" in i.keys()]
newList = [i for i in newList if len(i["Type"])]
if len(newList):
  jIn["SegmentationCodes"]["Category"] = newList
else:
  jIn["SegmentationCodes"].pop("Category")

print json.dumps(jIn,indent=2)
