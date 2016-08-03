from xmljson import badgerfish as bf
import xml.etree.ElementTree as ET
import sys, xmljson
from json import dumps

tree = ET.parse(sys.argv[1])
print dumps(xmljson.yahoo.data(tree.getroot()),indent=2)

