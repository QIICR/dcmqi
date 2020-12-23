/* Copyright (c) 2001-2020, David A. Clunie DBA Pixelmed Publishing. All rights reserved. */

package com.pixelmed.utils;

/**
 * <p>Various static methods helpful for color conversions.</p>
 *
 * @author	dclunie
 */
public class ColorUtilities {
	private static final String identString = "@(#) $Header: /userland/cvs/pixelmed/imgbook/com/pixelmed/utils/ColorUtilities.java,v 1.10 2020/01/01 15:48:26 dclunie Exp $";

	private ColorUtilities() {}
		
	/**
	 * <p>Convert floating point CIELab values to the 16 bit fractional integer scaled representation used in ICC profiles and DICOM.</p>
	 *
	 * <p>See ICC v4.3 Tables 12 and 13, and DICOM PS 3.3 C.10.7.1.1.</p>
	 *
	 * @param	cieLab	array of length 3 containing L*,a*,b* values with L* from 0.0 to 100.0, and a* and b* from -128.0 to +127.0
	 * return			array of length 3 containing 16 bit scaled L*,a*,b* values from  0 to 65535
	 */
	public static int[] getIntegerScaledCIELabFromCIELab(float[] cieLab) {
		int[] cieLabScaled = new int[3];
		// per PS 3.3 C.10.7.1.1 ... scale same way as ICC profile encodes them
		cieLabScaled[0] = (int)Math.round (cieLab[0] * 65535 / 100);
		cieLabScaled[1] = (int)Math.round((cieLab[1] + 128) * 65535 / 255);
		cieLabScaled[2] = (int)Math.round((cieLab[2] + 128) * 65535 / 255);
//System.err.println("CIELab ("+cieLab[0]+","+cieLab[1]+","+cieLab[2]+") -> CIELab scaled ("+cieLabScaled[0]+","+cieLabScaled[1]+","+cieLabScaled[2]+")");
		return cieLabScaled;
	}
	
	/**
	 * <p>Convert 16 bit fractional integer scaled CIELab values used in ICC profiles and DICOM to floating point.</p>
	 *
	 * <p>See ICC v4.3 Tables 12 and 13, and DICOM PS 3.3 C.10.7.1.1.</p>
	 *
	 * @param	cieLabScaled	array of length 3 containing 16 bit scaled L*,a*,b* values from  0 to 65535
	 * return					array of length 3 containing L*,a*,b* values with L* from 0.0 to 100.0, and a* and b* from -128.0 to +127.0
	 */
	public static float[] getCIELabPCSFromIntegerScaledCIELabPCS(int[] cieLabScaled) {
		float[] cieLab = new float[3];
		cieLab[0] = (float)(((double)cieLabScaled[0]) / 65535 * 100);
		cieLab[1] = (float)(((double)cieLabScaled[1]) / 65535 * 255 - 128);
		cieLab[2] = (float)(((double)cieLabScaled[2]) / 65535 * 255 - 128);
//System.err.println("CIELab scaled ("+cieLabScaled[0]+","+cieLabScaled[1]+","+cieLabScaled[2]+") -> CIELab ("+cieLab[0]+","+cieLab[1]+","+cieLab[2]+")");
		return cieLab;
	}

	/**
	 * <p>Convert CIEXYZ to CIE 1976 L*, a*, b*.</p>
	 *
	 * @see <a href="http://en.wikipedia.org/wiki/Lab_color_space#CIELAB-CIEXYZ_conversions">Wikipedia CIELAB-CIEXYZ_conversions</a>
	 *
	 * @param	cieXYZ	array of length 3 containing X,Y,Z values
	 * return			array of length 3 containing L*,a*,b* values
	 */
	public static float[] getCIELabFromXYZ(float[] cieXYZ) {
		// per http://www.easyrgb.com/index.php?X=MATH&H=07#text7
			
		double var_X = cieXYZ[0] / 95.047;		//ref_X =  95.047   Observer= 2°, Illuminant= D65
		double var_Y = cieXYZ[1] / 100.000;		//ref_Y = 100.000
		double var_Z = cieXYZ[2] / 108.883;		//ref_Z = 108.883

		if ( var_X > 0.008856 ) var_X = Math.pow(var_X,1.0/3);
		else                    var_X = ( 7.787 * var_X ) + ( 16.0 / 116 );
			
		if ( var_Y > 0.008856 ) var_Y = Math.pow(var_Y,1.0/3);
		else                    var_Y = ( 7.787 * var_Y ) + ( 16.0 / 116 );
			
		if ( var_Z > 0.008856 ) var_Z = Math.pow(var_Z,1.0/3);
		else                    var_Z = ( 7.787 * var_Z ) + ( 16.0 / 116 );

		float[] cieLab = new float[3];
		cieLab[0] = (float)(( 116 * var_Y ) - 16);			// CIE-L*
		cieLab[1] = (float)(500 * ( var_X - var_Y ));		// CIE-a*
		cieLab[2] = (float)(200 * ( var_Y - var_Z ));		// CIE-b*

//System.err.println("CIEXYZ ("+cieXYZ[0]+","+cieXYZ[1]+","+cieXYZ[2]+") -> CIELab ("+cieLab[0]+","+cieLab[1]+","+cieLab[2]+")");
		return cieLab;
	}
	
	/**
	 * <p>Convert CIE 1976 L*, a*, b* to CIEXYZ.</p>
	 *
	 * @see <a href="http://en.wikipedia.org/wiki/Lab_color_space#CIELAB-CIEXYZ_conversions">Wikipedia CIELAB-CIEXYZ_conversions</a>
	 *
	 * @param	cieLab	array of length 3 containing L*,a*,b* values
	 * return			array of length 3 containing X,Y,Z values
	 */
	public static float[] getCIEXYZFromLAB(float[] cieLab) {
		// per http://www.easyrgb.com/index.php?X=MATH&H=08#text8

		double var_Y = ( cieLab[0] + 16 ) / 116;
		double var_X = cieLab[1] / 500 + var_Y;
		double var_Z = var_Y - cieLab[2] / 200;
		
//System.err.println("var_Y = "+var_Y);
//System.err.println("var_X = "+var_X);
//System.err.println("var_Z = "+var_Z);

		double var_Y_pow3 = Math.pow(var_Y,3.0);
		double var_X_pow3 = Math.pow(var_X,3.0);
		double var_Z_pow3 = Math.pow(var_Z,3.0);

//System.err.println("var_Y_pow3 = "+var_Y_pow3);
//System.err.println("var_X_pow3 = "+var_X_pow3);
//System.err.println("var_Z_pow3 = "+var_Z_pow3);

		if (var_Y_pow3 > 0.008856) var_Y = var_Y_pow3;
		else                       var_Y = ( var_Y - 16d / 116 ) / 7.787;
		
		if (var_X_pow3 > 0.008856) var_X = var_X_pow3;
		else                       var_X = ( var_X - 16d / 116 ) / 7.787;
		
		if (var_Z_pow3 > 0.008856) var_Z = var_Z_pow3;
		else                       var_Z = ( var_Z - 16d / 116 ) / 7.787;

//System.err.println("var_Y = "+var_Y);
//System.err.println("var_X = "+var_X);
//System.err.println("var_Z = "+var_Z);

		float[] cieXYZ = new float[3];
		cieXYZ[0] = (float)(95.047  * var_X);     //ref_X =  95.047     Observer= 2°, Illuminant= D65
		cieXYZ[1] = (float)(100.000 * var_Y);     //ref_Y = 100.000
		cieXYZ[2] = (float)(108.883 * var_Z);     //ref_Z = 108.883

//System.err.println("CIELab ("+cieLab[0]+","+cieLab[1]+","+cieLab[2]+") -> CIEXYZ ("+cieXYZ[0]+","+cieXYZ[1]+","+cieXYZ[2]+")");
		return cieXYZ;
	}
		
	/**
	 * <p>Convert RGB values in sRGB to CIEXYZ in ICC PCS.</p>
	 *
	 * <p>SRGB Observer = 2°, Illuminant = D65, XYZ PCS Illuminant = D50.</p>
	 *
	 * @see <a href="http://en.wikipedia.org/wiki/SRGB#The_reverse_transformation">Wikipedia SRGB Reverse Transformation</a>
	 *
	 * @param	rgb		array of length 3 containing R,G,B values each from 0 to 255
	 * return			array of length 3 containing X,Y,Z values
	 */
	public static float[] getCIEXYZPCSFromSRGB(int[] rgb) {
		// per http://www.easyrgb.com/index.php?X=MATH&H=02#text2
			
		double var_R = ((double)rgb[0])/255.0;
		double var_G = ((double)rgb[1])/255.0;
		double var_B = ((double)rgb[2])/255.0;

		if ( var_R > 0.04045 ) var_R = Math.pow((var_R+0.055)/1.055,2.4);
		else                   var_R = var_R / 12.92;
			
		if ( var_G > 0.04045 ) var_G = Math.pow((var_G+0.055)/1.055,2.4);
		else                   var_G = var_G / 12.92;
			
		if ( var_B > 0.04045 ) var_B = Math.pow((var_B+0.055)/1.055,2.4);
		else                   var_B = var_B / 12.92;

		var_R = var_R * 100;
		var_G = var_G * 100;
		var_B = var_B * 100;

		float[] cieXYZ = new float[3];

		// SRGB Observer = 2°, Illuminant = D65, XYZ PCS Illuminant = D50
		cieXYZ[0] = (float)(var_R * 0.4124 + var_G * 0.3576 + var_B * 0.1805);
		cieXYZ[1] = (float)(var_R * 0.2126 + var_G * 0.7152 + var_B * 0.0722);
		cieXYZ[2] = (float)(var_R * 0.0193 + var_G * 0.1192 + var_B * 0.9505);
			
//System.err.println("RGB ("+rgb[0]+","+rgb[1]+","+rgb[2]+") -> CIEXYZ ("+cieXYZ[0]+","+cieXYZ[1]+","+cieXYZ[2]+")");
		return cieXYZ;
	}
	
	/**
	 * <p>Convert CIEXYZ in ICC PCS to RGB values in sRGB.</p>
	 *
	 * <p>SRGB Observer = 2°, Illuminant = D65, XYZ PCS Illuminant = D50.</p>
	 *
	 * @see <a href="http://en.wikipedia.org/wiki/SRGB#The_forward_transformation_.28CIE_xyY_or_CIE_XYZ_to_sRGB.29">Wikipedia SRGB Forward Transformation</a>
	 *
	 * @param	cieXYZ	array of length 3 containing X,Y,Z values
	 * return			array of length 3 containing R,G,B values each from 0 to 255
	 */
	public static int[] getSRGBFromCIEXYZPCS(float[] cieXYZ) {
		// per http://www.easyrgb.com/index.php?X=MATH&H=01#text1

		double var_X = cieXYZ[0] / 100;        //X from 0 to  95.047      (Observer = 2°, Illuminant = D65)
		double var_Y = cieXYZ[1] / 100;        //Y from 0 to 100.000
		double var_Z = cieXYZ[2] / 100;        //Z from 0 to 108.883

		double var_R = var_X *  3.2406 + var_Y * -1.5372 + var_Z * -0.4986;
		double var_G = var_X * -0.9689 + var_Y *  1.8758 + var_Z *  0.0415;
		double var_B = var_X *  0.0557 + var_Y * -0.2040 + var_Z *  1.0570;

		if ( var_R > 0.0031308 ) var_R = 1.055 * Math.pow(var_R,1/2.4) - 0.055;
		else                     var_R = 12.92 * var_R;
		
		if ( var_G > 0.0031308 ) var_G = 1.055 * Math.pow(var_G,1/2.4) - 0.055;
		else                     var_G = 12.92 * var_G;
		
		if ( var_B > 0.0031308 ) var_B = 1.055 * Math.pow(var_B,1/2.4) - 0.055;
		else                     var_B = 12.92 * var_B;

		int[] rgb = new int[3];
		rgb[0] = (int)Math.round(var_R * 255);
		rgb[1] = (int)Math.round(var_G * 255);
		rgb[2] = (int)Math.round(var_B * 255);
//System.err.println("CIEXYZ ("+cieXYZ[0]+","+cieXYZ[1]+","+cieXYZ[2]+") -> RGB ("+rgb[0]+","+rgb[1]+","+rgb[2]+")");
		return rgb;
	}
		
	/**
	 * <p>Convert RGB values in sRGB to CIELab in ICC PCS.</p>
	 *
	 * @param	rgb		array of length 3 containing R,G,B values each from 0 to 255
	 * return			array of length 3 containing L*,a*,b* values
	 */
	public static float[] getCIELabPCSFromSRGB(int[] rgb) {
		return getCIELabFromXYZ(getCIEXYZPCSFromSRGB(rgb));
	}

	/**
	 * <p>Convert RGB values in sRGB to 16 bit fractional integer scaled CIELab in ICC PCS.</p>
	 *
	 * @param	rgb		array of length 3 containing R,G,B values each from 0 to 255
	 * return			array of length 3 containing L*,a*,b* values each from 0 to 65535
	 */
	public static int[] getIntegerScaledCIELabPCSFromSRGB(int[] rgb) {
		return getIntegerScaledCIELabFromCIELab(getCIELabPCSFromSRGB(rgb));
	}
	
	/**
	 * <p>Convert CIELab in ICC PCS to RGB values in sRGB.</p>
	 *
	 * @param	cieLab	array of length 3 containing L*,a*,b* values
	 * return			array of length 3 containing R,G,B values each from 0 to 255
	 */
	public static int[] getSRGBFromCIELabPCS(float[] cieLab) {
		return getSRGBFromCIEXYZPCS(getCIEXYZFromLAB(cieLab));
	}

	/**
	 * <p>Convert 16 bit fractional integer scaled CIELab in ICC PCS to RGB values in sRGB.</p>
	 *
	 * @param	cieLabScaled	array of length 3 containing L*,a*,b* values each from 0 to 65535
	 * return					array of length 3 containing R,G,B values each from 0 to 255
	 */
	public static int[] getSRGBFromIntegerScaledCIELabPCS(int[] cieLabScaled) {
		return getSRGBFromCIELabPCS(getCIELabPCSFromIntegerScaledCIELabPCS(cieLabScaled));
	}

	
	/**
	 * <p>Convert color values</p>
	 *
	 * @param	arg	sRGB8toCIELab16 or CIELab16tosRGB8 (case insensitive) and three color values (each decimal or 0xhex)
	 */
	public static void main(String arg[]) {
		boolean bad = true;
		try {
			if (arg.length == 4) {
				int[] input = new int[3];
				input[0] = arg[1].startsWith("0x") ? Integer.parseInt(arg[1].replaceFirst("0x",""),16) :  Integer.parseInt(arg[1]);
				input[1] = arg[2].startsWith("0x") ? Integer.parseInt(arg[2].replaceFirst("0x",""),16) :  Integer.parseInt(arg[2]);
				input[2] = arg[3].startsWith("0x") ? Integer.parseInt(arg[3].replaceFirst("0x",""),16) :  Integer.parseInt(arg[3]);
				int[] output = null;
				String inputType = null;
				String outputType = null;
				if (arg[0].toLowerCase(java.util.Locale.US).equals("sRGB8toCIELab16".toLowerCase(java.util.Locale.US))) {
					output = getIntegerScaledCIELabPCSFromSRGB(input);
					inputType = "sRGB8";
					outputType = "CIELab16";
					bad = false;
				}
				else if (arg[0].toLowerCase(java.util.Locale.US).equals("CIELab16tosRGB8".toLowerCase(java.util.Locale.US))) {
					output = getSRGBFromIntegerScaledCIELabPCS(input);
					inputType = "CIELab16";
					outputType = "sRGB8";
					bad = false;
				}
				else {
					System.err.println("Unrecognized conversion type "+arg[0]);
				}
				if (output != null) {
					System.err.println(inputType+": "+input[0]+" "+input[1]+" "+input[2]+" (dec) (0x"+Integer.toHexString(input[0])+" 0x"+Integer.toHexString(input[1])+" 0x"+Integer.toHexString(input[2])+")");
					System.err.println(outputType+": "+output[0]+" "+output[1]+" "+output[2]+" (dec) (0x"+Integer.toHexString(output[0])+" 0x"+Integer.toHexString(output[1])+" 0x"+Integer.toHexString(output[2])+")");
				}
			}
			else {
				System.err.println("Error: incorrect number of arguments");
			}
		} catch (Exception e) {
			e.printStackTrace(System.err);	// no need to use SLF4J since command line utility/test
		}
		if (bad) {
			System.err.println("Usage: ColorUtilities sRGB8toCIELab16|CIELab16tosRGB8 R|L G|a B|b");
		}
	}
}


