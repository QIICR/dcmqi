import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

import jsoncompare


class TestJSONCompare(unittest.TestCase):
    def test_equal_nested_values(self):
        value = {'segments': [{'id': 1, 'data': [1, 2, 2]}], 'empty': None}
        self.assertTrue(jsoncompare.are_same(value, value)[0])

    def test_extra_dictionary_key_is_not_equal(self):
        self.assertFalse(jsoncompare.are_same({'a': 1}, {'a': 1, 'b': 2})[0])

    def test_missing_dictionary_key_is_not_equal(self):
        self.assertFalse(jsoncompare.are_same({'a': 1, 'b': 2}, {'a': 1})[0])

    def test_equal_length_different_dictionary_keys(self):
        self.assertFalse(jsoncompare.are_same({'a': 1}, {'b': 1})[0])

    def test_nested_extra_key_is_not_equal(self):
        self.assertFalse(jsoncompare.are_same({'a': {'b': 1}},
                                             {'a': {'b': 1, 'c': 2}})[0])

    def test_extra_list_item_is_not_equal(self):
        self.assertFalse(jsoncompare.are_same([1], [1, 2])[0])

    def test_short_duplicate_list_is_not_equal(self):
        self.assertFalse(jsoncompare.are_same([1, 1], [1])[0])

    def test_nested_list_length_is_checked(self):
        self.assertFalse(jsoncompare.are_same({'a': [1]}, {'a': [1, 1]})[0])

    def test_ignored_dictionary_keys_in_either_operand(self):
        for a, b in [({'x': 1}, {'x': 1, '@schema': 'new'}),
                     ({'x': 1, '@schema': 'old'}, {'x': 1}),
                     ({'x': 1, '@schema': 'old'},
                      {'x': 1, '@schema': 'new'})]:
            with self.subTest(a=a, b=b):
                self.assertTrue(jsoncompare.are_same(
                    a, b, ignore_value_of_keys=['@schema'])[0])

    def test_ignored_key_does_not_hide_unexpected_key(self):
        self.assertFalse(jsoncompare.are_same(
            {'a': 1, '@schema': 'old'}, {'a': 1, 'extra': 2},
            ignore_value_of_keys=['@schema'])[0])

    def test_contains_keeps_nested_subset_semantics(self):
        expected = {'a': {'b': 1}, 'values': [1]}
        actual = {'a': {'b': 1, 'c': 2}, 'values': [1, 2], 'extra': 3}
        self.assertTrue(jsoncompare.contains(expected, actual)[0])
        self.assertTrue(jsoncompare.are_same(
            expected, actual, ignore_missing_keys=True)[0])

    def test_contains_rejects_missing_expected_data(self):
        for expected, actual in [({'a': 1}, {}), ([1, 1], [1]),
                                 ({'a': [1, 1]}, {'a': [1]})]:
            with self.subTest(expected=expected, actual=actual):
                self.assertFalse(jsoncompare.contains(expected, actual)[0])

    def test_list_order_and_duplicate_counts(self):
        self.assertFalse(jsoncompare.are_same([1, 2], [2, 1])[0])
        self.assertTrue(jsoncompare.are_same(
            [1, 2], [2, 1], ignore_list_order_recursively=True)[0])
        self.assertFalse(jsoncompare.are_same(
            [1, 1], [1], ignore_list_order_recursively=True)[0])

    def test_type_checks_are_unchanged(self):
        for expected, actual in [(True, 1), (1, 1.0), ([], {}), (None, 0)]:
            with self.subTest(expected=expected, actual=actual):
                self.assertFalse(jsoncompare.are_same(expected, actual)[0])

    def test_json_wrapper_forwards_ignored_keys(self):
        self.assertTrue(jsoncompare.json_are_same(
            '{"a": 1, "@schema": "old"}', '{"a": 1}',
            ignore_value_of_keys=['@schema'])[0])
        self.assertFalse(jsoncompare.json_are_same(
            '{"a": 1}', '{"a": 1, "extra": 2}',
            ignore_value_of_keys=['@schema'])[0])

    def test_cli_strict_exit_status(self):
        script = Path(jsoncompare.__file__).with_name('comparejson.py')
        cases = [({'a': 1}, {'a': 1}, 0),
                 ({'a': 1}, {'a': 1, '@schema': 'new'}, 0),
                 ({'a': 1}, {'a': 1, 'extra': 2}, 1),
                 ({'a': {'b': 1}}, {'a': {'b': 1, 'extra': 2}}, 1),
                 ({'a': [1]}, {'a': [1, 1]}, 1)]
        with tempfile.TemporaryDirectory() as directory:
            left = Path(directory) / 'expected.json'
            right = Path(directory) / 'actual.json'
            for expected, actual, code in cases:
                with self.subTest(expected=expected, actual=actual):
                    left.write_text(json.dumps(expected), encoding='utf-8')
                    right.write_text(json.dumps(actual), encoding='utf-8')
                    result = subprocess.run(
                        [sys.executable, str(script), str(left), str(right)],
                        capture_output=True, text=True, check=False)
                    self.assertEqual(result.returncode, code, result.stdout)


if __name__ == '__main__':
    unittest.main()
