# helper script to populate Slicer integer labels in the
#   segmentation context JSON file from Slicer LUT CSV file
#
# Usage:
# $ python resources/add_slicer_ids.py resources/SlicerGenericAnatomy.csv \
#   SegmentationCategoryTypeModifier-SlicerGeneralAnatomy.json \
#   SegmentationCategoryTypeModifier-SlicerGeneralAnatomy_updated.json

import csv, sys, pandas, json, copy

def lookup_slicer_int_from_text(slicer_lut, slicer_text_label):
  try:
    slicer_int_label = int(slicer_lut[slicer_lut["Text Label"] == slicer_text_label]["Integer Label"].values[0])
  except IndexError:
    print("Failed to find "+slicer_text_label+" in the CSV file")
    raise IndexError

  return slicer_int_label

slicer_lut = pandas.read_csv(sys.argv[1])
slicer_lut = slicer_lut.apply(lambda x: x.str.strip() if x.dtype == "object" else x)

with open(sys.argv[2], 'r') as json_file:
  seg_context = json.load(json_file)

new_seg_context = copy.deepcopy(seg_context)

for category_id,category in enumerate(seg_context["SegmentationCodes"]["Category"]):
  for type_id,type in enumerate(category["Type"]):
    try:
      slicer_label = type["3dSlicerLabel"]
    except KeyError:
      if not "Modifier" in type.keys(): # otherwise it's not a concern
        print("Failed to find 3dSlicerLabel in "+str(type))

    slicer_int_label = lookup_slicer_int_from_text(slicer_lut, slicer_label)
    new_seg_context["SegmentationCodes"]["Category"][category_id]["Type"][type_id]["3dSlicerIntegerLabel"] = slicer_int_label

    if "Modifier" in type.keys():
      for modifier_id,modifier in enumerate(type["Modifier"]):
        try:
          slicer_label = modifier["3dSlicerLabel"]
        except KeyError:
          print("Failed to find 3dSlicerLabel in modifier for "+json.dumps(type, indent=2))
        slicer_int_label = lookup_slicer_int_from_text(slicer_lut, slicer_label)
        new_seg_context["SegmentationCodes"]["Category"][category_id]["Type"][type_id]["Modifier"][modifier_id]["3dSlicerIntegerLabel"] = slicer_int_label

with open(sys.argv[3],"w") as new_seg_context_file:
  json.dump(new_seg_context, new_seg_context_file, indent=2)
