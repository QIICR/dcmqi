import json, sys
import jsoncompare
import ast


if len(sys.argv) < 3:
  sys.exit('Error')
json1 = json.loads(open(sys.argv[1],'r').read())
json2 = json.loads(open(sys.argv[2],'r').read())

ignoredKeys = ["@schema"]

try:
    ignoredKeys += ast.literal_eval(sys.argv[3])
except IndexError:
    pass

json2 = json.loads(open(sys.argv[2],'r').read())

(check,stack) = jsoncompare.are_same(json1,json2,
                                     ignore_list_order_recursively=False,
                                     ignore_missing_keys=False,
                                     ignore_value_of_keys=ignoredKeys)

if not check:
  print(stack)
  sys.exit(1)
