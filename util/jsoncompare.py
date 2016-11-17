import json


class Stack:
    def __init__(self):
        self.stack_items = []

    def append(self, stack_item):
        self.stack_items.append(stack_item)
        return self

    def __repr__(self):
        stack_dump = ''
        for item in self.stack_items:
            stack_dump += str(item)
        return stack_dump

    def __str__(self):
        stack_dump = ''
        for item in self.stack_items:
            stack_dump += str(item)
        return stack_dump


class StackItem:
    def __init__(self, reason, expected, actual):
        self.reason = reason
        self.expected = expected
        self.actual = actual

    def __repr__(self):
        return 'Reason: {0}\nExpected:\n{1}\nActual:\n{2}' \
               .format(self.reason, _format_value(self.expected), _format_value(self.actual))

    def __str__(self):
        return '\n\nReason: {0}\nExpected:\n{1}\nActual:\n{2}' \
               .format(self.reason, _format_value(self.expected), _format_value(self.actual))


def _indent(s):
    return '\n'.join('  ' + line for line in s.splitlines())


def _format_value(value):
    return _indent(_generate_pprint_json(value))


def _generate_pprint_json(value):
    return json.dumps(value, sort_keys=True, indent=4)


def _is_dict_same(expected, actual, ignore_value_of_keys):
    # DAN - I had to flip flop this
    for key in expected:
        if key in ignore_value_of_keys:
            continue
        if key not in actual:
            return False, \
                   Stack().append(
                        StackItem('Expected key "{0}" Missing from Actual'
                                      .format(key),
                                  expected,
                                  actual))

        are_same_flag, stack = _are_same(expected[key], actual[key],ignore_value_of_keys)
        if not are_same_flag:
            return False, \
                   stack.append(StackItem('Different values', expected[key], actual[key]))
    return True, Stack()


def _is_list_same(expected, actual, ignore_value_of_keys):
    for i in xrange(len(expected)):
        are_same_flag, stack = _are_same(expected[i], actual[i], ignore_value_of_keys)
        if not are_same_flag:
            return False, \
                   stack.append(
                       StackItem('Different values (Check order)', expected[i], actual[i]))
    return True, Stack()


def _bottom_up_sort(unsorted_json):
    if isinstance(unsorted_json, list):
        new_list = []
        for i in xrange(len(unsorted_json)):
            new_list.append(_bottom_up_sort(unsorted_json[i]))
        return sorted(new_list)

    elif isinstance(unsorted_json, dict):
        new_dict = {}
        for key in sorted(unsorted_json):
            new_dict[key] = _bottom_up_sort(unsorted_json[key])
        return new_dict

    else:
        return unsorted_json


def _are_same(expected, actual, ignore_value_of_keys, ignore_missing_keys=False):
    # Check for None
    if expected is None:
        return expected == actual, Stack()

    # Ensure they are of same type
    if type(expected) != type(actual):
        return False, \
               Stack().append(
                   StackItem('Type Mismatch: Expected Type: {0}, Actual Type: {1}'
                                .format(type(expected), type(actual)),
                             expected,
                             actual))

    # Compare primitive types immediately
    if type(expected) in (int, str, bool, long, float, unicode):
        return expected == actual, Stack()

    if not ignore_missing_keys and len(expected) > len(actual):
        all_missing_keys = get_missing_keys(expected, actual)
        missing_keys = [x for x in all_missing_keys if x not in ignore_value_of_keys]
        if len(missing_keys):
            stack = Stack().append(StackItem('Length Mismatch: Expected Length: {0}, Actual Length: {1}'
                                              .format(len(expected), len(actual)), expected, actual))
            if isinstance(expected, dict):
              stack.append('\nMissing keys: {0}'.format(missing_keys))
              stack.append('\nIgnored keys: {0}'.format([x for x in ignore_value_of_keys if x not in all_missing_keys]))
            return False, stack

    if isinstance(expected, dict):
        return _is_dict_same(expected, actual, ignore_value_of_keys)

    if isinstance(expected, list):
        return _is_list_same(expected, actual, ignore_value_of_keys)

    return False, Stack().append(StackItem('Unhandled Type: {0}'.format(type(expected)), expected, actual))


def get_missing_keys(expected, actual):
    return [key for key in expected if key not in actual]


def are_same(original_a, original_b, ignore_list_order_recursively=False, ignore_missing_keys=False, ignore_value_of_keys=None):
    ignore_value_of_keys = ignore_value_of_keys if ignore_value_of_keys else []
    a = _bottom_up_sort(original_a) if ignore_list_order_recursively else original_a
    b = _bottom_up_sort(original_b) if ignore_list_order_recursively else original_b
    return _are_same(a, b, ignore_value_of_keys, ignore_missing_keys)


def contains(expected_original, actual_original, ignore_list_order_recursively=False, ignore_value_of_keys=None):
    ignore_value_of_keys = ignore_value_of_keys if ignore_value_of_keys else []
    actual = _bottom_up_sort(actual_original) if ignore_list_order_recursively else actual_original
    expected = _bottom_up_sort(expected_original) if ignore_list_order_recursively else expected_original
    return _are_same(expected, actual, ignore_value_of_keys, True)


def json_are_same(a, b, ignore_list_order_recursively=False, ignore_value_of_keys=None):
    ignore_value_of_keys = ignore_value_of_keys if ignore_value_of_keys else []
    return are_same(json.loads(a), json.loads(b), ignore_list_order_recursively, ignore_value_of_keys)

