/* Copyright (c) 2001-2020, David A. Clunie DBA Pixelmed Publishing. All rights reserved. */

package com.pixelmed.test;

import com.pixelmed.utils.ColorUtilities;

import junit.framework.*;

public class TestColorConversions_SRGB_CIELabPCS extends TestCase {
	
	// constructor to support adding tests to suite ...
	
	public TestColorConversions_SRGB_CIELabPCS(String name) {
		super(name);
	}
	
	// add tests to suite manually, rather than depending on default of all test...() methods
	// in order to allow adding TestColorConversions_SRGB_CIELabPCS.suite() in AllTests.suite()
	// see Johannes Link. Unit Testing in Java pp36-47
	
	public static Test suite() {
		TestSuite suite = new TestSuite("TestColorConversions_SRGB_CIELabPCS");
		
		suite.addTest(new TestColorConversions_SRGB_CIELabPCS("TestColorConversions_SRGB_CIELabPCS_SpecificValues"));
		suite.addTest(new TestColorConversions_SRGB_CIELabPCS("TestColorConversions_SRGB_CIELabPCS_WhitePoint"));
		
		return suite;
	}
	
	protected void setUp() {
	}
	
	protected void tearDown() {
	}
	
	int[][] testRGBValues = {
		{ 0,0,0 },
		{ 255,0,0 },
		{ 0,255,0 },
		{ 0,0,255 },
		{ 255,255,0 },
		{ 0,255,255 },
		{ 255,0,255 },
		{ 255,255,255 },
		{ 225,190,150 },
		{ 200,200,200 },
		{ 128,174,128 },
		{ 221,130,101 }
	};
	
	public void TestColorConversions_SRGB_CIELabPCS_SpecificValues() throws Exception {
		for (int[] rgb : testRGBValues) {
			int[] lab = ColorUtilities.getIntegerScaledCIELabPCSFromSRGB(rgb);
System.err.println("TestColorConversions_SRGB_CIELabPCS_SpecificValues(): RGB ("+rgb[0]+","+rgb[1]+","+rgb[2]+") = Lab ("+lab[0]+","+lab[1]+","+lab[2]+")");
			int[] rgbRoundTrip = ColorUtilities.getSRGBFromIntegerScaledCIELabPCS(lab);
			assertEquals("Checking round trip r",rgb[0],rgbRoundTrip[0]);
			assertEquals("Checking round trip g",rgb[1],rgbRoundTrip[1]);
			assertEquals("Checking round trip b",rgb[2],rgbRoundTrip[2]);
		}
	}
	
	// ICC v4.3 Table 14

	public void TestColorConversions_SRGB_CIELabPCS_WhitePoint() throws Exception {
		{
			float[] lab = { 100f,0f,0f };
			int[] scaledExpect = { 0xffff,0x8080,0x8080 };
			int[] scale = ColorUtilities.getIntegerScaledCIELabFromCIELab(lab);
			assertEquals("Checking scaling L"+lab[0],scaledExpect[0],scale[0]);
			assertEquals("Checking scaling a"+lab[1],scaledExpect[1],scale[1]);
			assertEquals("Checking scaling b"+lab[2],scaledExpect[2],scale[2]);
		}
	}
	
}	
