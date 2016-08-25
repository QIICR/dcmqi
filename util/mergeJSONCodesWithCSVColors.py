import json, sys, csv, re

class Term:
  def __init__(self,s):
    #print s,' maps to ',
    try:
      mo = re.search("\((.*),(.*),(\".*\")\)",s)

      self.code = mo.group(1)
      self.codingScheme = mo.group(2)
      self.meaning = mo.group(3)
    except:
      self.code = ""
      self.codingScheme = ""
      self.meaning = ""
    #print self.getStr()
  def toStr(self):
    return '('+self.code+','+self.codingScheme+','+self.meaning+')'

class RGB:
  def __init__(self,s):
    try:
      mo = re.search("rgb\(([0-9]*),([0-9]*),([0-9]*)\)",s)
      self.rgb = [mo.group(1),mo.group(2),mo.group(3)]
    except:
      self.rgb = ""
  def toStr(self):
    return str(self.rgb)

def addColor(codeDict,jsonDict,c,t,m=-1):
  jsonCat = jsonCodes["SegmentationCodes"]["Category"][c]["codeValue"]
  jsonType = jsonCodes["SegmentationCodes"]["Category"][c]["Type"][t]["codeValue"]
  if m != -1:
    jsonMod = jsonCodes["SegmentationCodes"]["Category"][c]["Type"][t]["Modifier"][m]["codeValue"]
  else:
    jsonMod = ""
  key = jsonCat+","+jsonType+","+jsonMod

  if key in codeDict:
    rgb = codeDict[key][-2].rgb
    slicerName = codeDict[key][-3]
    codeDict[key][-1] = True # mark this entry as matched
    if m != -1:
      jsonCodes["SegmentationCodes"]["Category"][c]["Type"][t]["Modifier"][m]["recommendedDisplayRGBValue"] = rgb
      jsonCodes["SegmentationCodes"]["Category"][c]["Type"][t]["Modifier"][m]["3dSlicerLabel"] = slicerName
    else:
      jsonCodes["SegmentationCodes"]["Category"][c]["Type"][t]["recommendedDisplayRGBValue"] = rgb
      jsonCodes["SegmentationCodes"]["Category"][c]["Type"][t]["3dSlicerLabel"] = slicerName
    return True
  else:
    return False

def copyNonListItems(src,dest,destKey=None):
  if destKey:
    dest[destKey] = {}
  for k in src.keys():
    if type(k) != 'list':
      if not destKey:
        dest[k] = src[k]
      else:
        dest[destKey][k] = src[k]

if len(sys.argv)<3:
  print "Usage: ",sys.argv[0]," SegmentationCodesJSON SegmentationCodesCSV <outputJSON>"
  print "  where SegmentationCodesJSON is the SegmentationCategoryType JSON"
  print "  file from doc/segContext, and SegmentationCodesCSV is the CSV file" print "  imported from the first sheet here https://goo.gl/vuANnw"

inputJSONFileName = sys.argv[1]
inputCSVFileName = sys.argv[2]

if len(sys.argv)>3:
  outputJSONFileName = sys.argv[3]
else:
  outputJSONFileName = None

jsonCodes = json.loads(open(inputJSONFileName).read())
#jsonCodes = jsonCodes["SegmentationCodes"]

codeDict = {}

with open(inputCSVFileName) as csvfile:
  csvreader = csv.reader(csvfile)
  for row in csvreader:
    slicerName = row[1].strip()
    segCategory = Term(row[6])
    segType = Term(row[7])
    segModifier = Term(row[9])
    segRGB = RGB(row[10])
    #if segCategory != None and segCategory != ' ':
    #  print '*'+segCategory+'*',segType,segModifier,segRGB
    key = segCategory.code+","+segType.code+","+segModifier.code
    if segCategory.code != "":
      # last item indicates this entry has not been matched to the JSON dataset
      codeDict[key] = [segCategory,segType,segModifier,slicerName,segRGB,False]

print "Total valid entries in CSV:",len(codeDict.keys())

totalJSONEntries = 0
matchedCodes = []
for c in range(len(jsonCodes["SegmentationCodes"]["Category"])):
  for t in range(len(jsonCodes["SegmentationCodes"]["Category"][c]["Type"])):
    if "Modifier" in jsonCodes["SegmentationCodes"]["Category"][c]["Type"][t]:
      for m in range(len(jsonCodes["SegmentationCodes"]["Category"][c]["Type"][t]["Modifier"])):
        matched = addColor(codeDict,jsonCodes,c,t,m)
        totalJSONEntries = totalJSONEntries+1
        if matched:
          matchedJsonCodes.append([c,t,m])
    else:
      matched = addColor(codeDict,jsonCodes,c,t)
      totalJSONEntries = totalJSONEntries+1
      if matched:
        matchedJsonCodes.append([c,t])

# copy matched codes to a new JSON
matchedCodesJson = {}
matchedCodesJson["SegmentationCodes"] = {}
matchedCodesJson["SegmentationCodes"]["Category"] = {}
# look up if a category with the given key/coding scheme already exists
categoryLookupByKey = {}

if len(matchedCodes):
  for t in matchedJsonCodes:
    cCat = jsonCodes["SegmentationCodes"]["Category"][t[0]]]
    cType = jsonCodes["SegmentationCodes"]["Category"][t[0]]]["Type"][t[1]]
    if len(t)==3:
      cMod = jsonCodes["SegmentationCodes"]["Category"][t[0]]]["Type"][t[1]]["Modifier"][t[2]]
    else:
      cMod = None



print "Total JSON entries:",totalJSONEntries

unmatchedEntries = [l for l in codeDict.values() if l[-1]==False]
print "Total unmatched entries:",len(unmatchedEntries)

for e in unmatchedEntries:
  print e[0].toStr(),e[1].toStr(),e[2].toStr()

if outputJSONFileName:
  f = open(outputJSONFileName,"w")
  f.write(json.dumps(jsonCodes,indent=2))
