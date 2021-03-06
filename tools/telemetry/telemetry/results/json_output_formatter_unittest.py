# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import json
import os
import StringIO
import unittest

from telemetry import benchmark
from telemetry.page import page_set
from telemetry.results import json_output_formatter
from telemetry.results import page_test_results
from telemetry.value import scalar


def _MakePageSet():
  ps = page_set.PageSet(file_path=os.path.dirname(__file__))
  ps.AddPageWithDefaultRunNavigate('http://www.foo.com/')
  ps.AddPageWithDefaultRunNavigate('http://www.bar.com/')
  return ps

def _HasPage(pages, page):
  return pages.get(page.id, None) != None

def _HasValueNamed(values, name):
  return len([x for x in values if x['name'] == name]) == 1

class JsonOutputFormatterTest(unittest.TestCase):
  def setUp(self):
    self._output = StringIO.StringIO()
    self._page_set = _MakePageSet()
    self._formatter = json_output_formatter.JsonOutputFormatter(self._output,
        benchmark.BenchmarkMetadata('benchmark_name'))

  def testOutputAndParse(self):
    results = page_test_results.PageTestResults()

    self._output.truncate(0)

    results.WillRunPage(self._page_set[0])
    v0 = scalar.ScalarValue(results.current_page, 'foo', 'seconds', 3)
    results.AddValue(v0)
    results.DidRunPage(self._page_set[0])

    self._formatter.Format(results)
    json.loads(self._output.getvalue())

  def testAsDictBaseKeys(self):
    results = page_test_results.PageTestResults()
    d = json_output_formatter.ResultsAsDict(results,
        self._formatter.benchmark_metadata)

    self.assertEquals(d['format_version'], '0.2')
    self.assertEquals(d['benchmark_name'], 'benchmark_name')

  def testAsDictWithOnePage(self):
    results = page_test_results.PageTestResults()
    results.WillRunPage(self._page_set[0])
    v0 = scalar.ScalarValue(results.current_page, 'foo', 'seconds', 3)
    results.AddValue(v0)
    results.DidRunPage(self._page_set[0])

    d = json_output_formatter.ResultsAsDict(results,
        self._formatter.benchmark_metadata)

    self.assertTrue(_HasPage(d['pages'], self._page_set[0]))
    self.assertTrue(_HasValueNamed(d['per_page_values'], 'foo'))

  def testAsDictWithTwoPages(self):
    results = page_test_results.PageTestResults()
    results.WillRunPage(self._page_set[0])
    v0 = scalar.ScalarValue(results.current_page, 'foo', 'seconds', 3)
    results.AddValue(v0)
    results.DidRunPage(self._page_set[0])

    results.WillRunPage(self._page_set[1])
    v1 = scalar.ScalarValue(results.current_page, 'bar', 'seconds', 4)
    results.AddValue(v1)
    results.DidRunPage(self._page_set[1])

    d = json_output_formatter.ResultsAsDict(results,
        self._formatter.benchmark_metadata)

    self.assertTrue(_HasPage(d['pages'], self._page_set[0]))
    self.assertTrue(_HasPage(d['pages'], self._page_set[1]))
    self.assertTrue(_HasValueNamed(d['per_page_values'], 'foo'))
    self.assertTrue(_HasValueNamed(d['per_page_values'], 'bar'))

  def testAsDictWithSummaryValueOnly(self):
    results = page_test_results.PageTestResults()
    v = scalar.ScalarValue(None, 'baz', 'seconds', 5)
    results.AddSummaryValue(v)

    d = json_output_formatter.ResultsAsDict(results,
        self._formatter.benchmark_metadata)

    self.assertFalse(d['pages'])
    self.assertTrue(_HasValueNamed(d['summary_values'], 'baz'))
