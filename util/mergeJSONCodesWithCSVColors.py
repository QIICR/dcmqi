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
    codeDict[key][-1] = True # mark this entry as matched
    if m != -1:
      jsonCodes["SegmentationCodes"]["Category"][c]["Type"][t]["Modifier"][m]["recommendedDisplayRGBValue"] = rgb
    else:
      jsonCodes["SegmentationCodes"]["Category"][c]["Type"][t]["recommendedDisplayRGBValue"] = rgb
    return True
  else:
    return False

if len(sys.argv)<3:
  print "Usage: ",sys.argv[0]," AnatomicCodesJSON AnatomicCodesCSV"
  print "  where AnatomicCodesJSON is the JSON file from doc/segContext"
  print "  and AnatomicCodesCSV is the CSV file imported from the first sheet here https://goo.gl/vuANnw"

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
    segCategory = Term(row[6])
    segType = Term(row[7])
    segModifier = Term(row[9])
    segRGB = RGB(row[10])
    #if segCategory != None and segCategory != ' ':
    #  print '*'+segCategory+'*',segType,segModifier,segRGB
    key = segCategory.code+","+segType.code+","+segModifier.code
    if segCategory.code != "":
      # last item indicates this entry has not been matched to the JSON dataset
      codeDict[key] = [segCategory,segType,segModifier,segRGB,False]

print "Total valid entries in CSV:",len(codeDict.keys())

totalJSONEntries = 0
for c in range(len(jsonCodes["SegmentationCodes"]["Category"])):
  for t in range(len(jsonCodes["SegmentationCodes"]["Category"][c]["Type"])):
    if "Modifier" in jsonCodes["SegmentationCodes"]["Category"][c]["Type"][t]:
      for m in range(len(jsonCodes["SegmentationCodes"]["Category"][c]["Type"][t]["Modifier"])):
        addColor(codeDict,jsonCodes,c,t,m)
        totalJSONEntries = totalJSONEntries+1
    else:
      addColor(codeDict,jsonCodes,c,t)
      totalJSONEntries = totalJSONEntries+1

print "Total JSON entries:",totalJSONEntries

unmatchedEntries = [l for l in codeDict.values() if l[-1]==False]
print "Total unmatched entries:",len(unmatchedEntries)

for e in unmatchedEntries:
  print e[0].toStr(),e[1].toStr(),e[2].toStr()

if outputJSONFileName:
  f = open(outputJSONFileName,"w")
  f.write(json.dumps(jsonCodes,indent=2))
