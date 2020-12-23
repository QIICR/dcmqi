#include "dcmqi/ColorUtilities.h"

#include <math.h>

namespace dcmqi {

  void ColorUtilities::getIntegerScaledCIELabFromCIELab(int &sL, int &sA, int &sB, float L, float A, float B) {
    // per PS 3.3 C.10.7.1.1 ... scale same way as ICC profile encodes them
    sL = (int)round (L * 65535 / 100);
    sA = (int)round((A + 128) * 65535 / 255);
    sB = (int)round((B + 128) * 65535 / 255);
//System.err.println("CIELab ("+cieLab[0]+","+cieLab[1]+","+cieLab[2]+") -> CIELab scaled ("+cieLabScaled[0]+","+cieLabScaled[1]+","+cieLabScaled[2]+")");
  }

  /**
   * <p>Convert 16 bit fractional integer scaled CIELab values used in ICC profiles and DICOM to floating point.</p>
   *
   * <p>See ICC v4.3 Tables 12 and 13, and DICOM PS 3.3 C.10.7.1.1.</p>
   *
   * @param	cieLabScaled	array of length 3 containing 16 bit scaled L*,a*,b* values from  0 to 65535
   * return					array of length 3 containing L*,a*,b* values with L* from 0.0 to 100.0, and a* and b* from -128.0 to +127.0
   */
  void ColorUtilities::getCIELabPCSFromIntegerScaledCIELabPCS(float &L, float &A, float &B, int sL, int sA, int sB) {
    L = (float)(((double)sL) / 65535 * 100);
    A = (float)(((double)sA) / 65535 * 255 - 128);
    B = (float)(((double)sB) / 65535 * 255 - 128);
//System.err.println("CIELab scaled ("+cieLabScaled[0]+","+cieLabScaled[1]+","+cieLabScaled[2]+") -> CIELab ("+cieLab[0]+","+cieLab[1]+","+cieLab[2]+")");
  }

  /**
   * <p>Convert CIEXYZ to CIE 1976 L*, a*, b*.</p>
   *
   * @see <a href="http://en.wikipedia.org/wiki/Lab_color_space#CIELAB-CIEXYZ_conversions">Wikipedia CIELAB-CIEXYZ_conversions</a>
   *
   * @param	cieXYZ	array of length 3 containing X,Y,Z values
   * return			array of length 3 containing L*,a*,b* values
   */
  void ColorUtilities::getCIELabFromXYZ(float &L, float &A, float &B, float X, float Y, float Z) {
    // per http://www.easyrgb.com/index.php?X=MATH&H=07#text7

    double var_X = X / 95.047;		//ref_X =  95.047   Observer= 2°, Illuminant= D65
    double var_Y = Y / 100.000;		//ref_Y = 100.000
    double var_Z = Z / 108.883;		//ref_Z = 108.883

    if ( var_X > 0.008856 ) var_X = pow(var_X,1.0/3);
    else                    var_X = ( 7.787 * var_X ) + ( 16.0 / 116 );

    if ( var_Y > 0.008856 ) var_Y = pow(var_Y,1.0/3);
    else                    var_Y = ( 7.787 * var_Y ) + ( 16.0 / 116 );

    if ( var_Z > 0.008856 ) var_Z = pow(var_Z,1.0/3);
    else                    var_Z = ( 7.787 * var_Z ) + ( 16.0 / 116 );

    L = (float)(( 116 * var_Y ) - 16);			// CIE-L*
    A = (float)(500 * ( var_X - var_Y ));		// CIE-a*
    B = (float)(200 * ( var_Y - var_Z ));		// CIE-b*

//System.err.println("CIEXYZ ("+cieXYZ[0]+","+cieXYZ[1]+","+cieXYZ[2]+") -> CIELab ("+cieLab[0]+","+cieLab[1]+","+cieLab[2]+")");
  }

  /**
   * <p>Convert CIE 1976 L*, a*, b* to CIEXYZ.</p>
   *
   * @see <a href="http://en.wikipedia.org/wiki/Lab_color_space#CIELAB-CIEXYZ_conversions">Wikipedia CIELAB-CIEXYZ_conversions</a>
   *
   * @param	cieLab	array of length 3 containing L*,a*,b* values
   * return			array of length 3 containing X,Y,Z values
   */
  void ColorUtilities::getCIEXYZFromLAB(float &X, float &Y, float &Z, float L, float A, float B) {
    // per http://www.easyrgb.com/index.php?X=MATH&H=08#text8

    double var_Y = ( L + 16 ) / 116;
    double var_X = A / 500 + var_Y;
    double var_Z = var_Y - B / 200;

//System.err.println("var_Y = "+var_Y);
//System.err.println("var_X = "+var_X);
//System.err.println("var_Z = "+var_Z);

    double var_Y_pow3 = pow(var_Y,3.0);
    double var_X_pow3 = pow(var_X,3.0);
    double var_Z_pow3 = pow(var_Z,3.0);

//System.err.println("var_Y_pow3 = "+var_Y_pow3);
//System.err.println("var_X_pow3 = "+var_X_pow3);
//System.err.println("var_Z_pow3 = "+var_Z_pow3);

    if (var_Y_pow3 > 0.008856) var_Y = var_Y_pow3;
    else                       var_Y = ( var_Y - 16 / 116 ) / 7.787;

    if (var_X_pow3 > 0.008856) var_X = var_X_pow3;
    else                       var_X = ( var_X - 16 / 116 ) / 7.787;

    if (var_Z_pow3 > 0.008856) var_Z = var_Z_pow3;
    else                       var_Z = ( var_Z - 16 / 116 ) / 7.787;

//System.err.println("var_Y = "+var_Y);
//System.err.println("var_X = "+var_X);
//System.err.println("var_Z = "+var_Z);

    X = (float)(95.047  * var_X);     //ref_X =  95.047     Observer= 2°, Illuminant= D65
    Y = (float)(100.000 * var_Y);     //ref_Y = 100.000
    Z = (float)(108.883 * var_Z);     //ref_Z = 108.883

//System.err.println("CIELab ("+cieLab[0]+","+cieLab[1]+","+cieLab[2]+") -> CIEXYZ ("+cieXYZ[0]+","+cieXYZ[1]+","+cieXYZ[2]+")");
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
  void ColorUtilities::getCIEXYZPCSFromSRGB(float &X, float &Y, float &Z, int R, int G, int B) {
    // per http://www.easyrgb.com/index.php?X=MATH&H=02#text2

    double var_R = ((double)R)/255.0;
    double var_G = ((double)G)/255.0;
    double var_B = ((double)B)/255.0;

    if ( var_R > 0.04045 ) var_R = pow((var_R+0.055)/1.055,2.4);
    else                   var_R = var_R / 12.92;

    if ( var_G > 0.04045 ) var_G = pow((var_G+0.055)/1.055,2.4);
    else                   var_G = var_G / 12.92;

    if ( var_B > 0.04045 ) var_B = pow((var_B+0.055)/1.055,2.4);
    else                   var_B = var_B / 12.92;

    var_R = var_R * 100;
    var_G = var_G * 100;
    var_B = var_B * 100;

    // SRGB Observer = 2°, Illuminant = D65, XYZ PCS Illuminant = D50
    X = (float)(var_R * 0.4124 + var_G * 0.3576 + var_B * 0.1805);
    Y = (float)(var_R * 0.2126 + var_G * 0.7152 + var_B * 0.0722);
    Z = (float)(var_R * 0.0193 + var_G * 0.1192 + var_B * 0.9505);

//System.err.println("RGB ("+rgb[0]+","+rgb[1]+","+rgb[2]+") -> CIEXYZ ("+cieXYZ[0]+","+cieXYZ[1]+","+cieXYZ[2]+")");
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
  void ColorUtilities::getSRGBFromCIEXYZPCS(int &R, int &G, int &B, float X, float Y, float Z) {
    // per http://www.easyrgb.com/index.php?X=MATH&H=01#text1

    double var_X = X / 100;        //X from 0 to  95.047      (Observer = 2°, Illuminant = D65)
    double var_Y = Y / 100;        //Y from 0 to 100.000
    double var_Z = Z / 100;        //Z from 0 to 108.883

    double var_R = var_X *  3.2406 + var_Y * -1.5372 + var_Z * -0.4986;
    double var_G = var_X * -0.9689 + var_Y *  1.8758 + var_Z *  0.0415;
    double var_B = var_X *  0.0557 + var_Y * -0.2040 + var_Z *  1.0570;

    if ( var_R > 0.0031308 ) var_R = 1.055 * pow(var_R,1/2.4) - 0.055;
    else                     var_R = 12.92 * var_R;

    if ( var_G > 0.0031308 ) var_G = 1.055 * pow(var_G,1/2.4) - 0.055;
    else                     var_G = 12.92 * var_G;

    if ( var_B > 0.0031308 ) var_B = 1.055 * pow(var_B,1/2.4) - 0.055;
    else                     var_B = 12.92 * var_B;

    R = (int)round(var_R * 255);
    G = (int)round(var_G * 255);
    B = (int)round(var_B * 255);
//System.err.println("CIEXYZ ("+cieXYZ[0]+","+cieXYZ[1]+","+cieXYZ[2]+") -> RGB ("+rgb[0]+","+rgb[1]+","+rgb[2]+")");
  }

  /**
   * <p>Convert RGB values in sRGB to CIELab in ICC PCS.</p>
   *
   * @param	rgb		array of length 3 containing R,G,B values each from 0 to 255
   * return			array of length 3 containing L*,a*,b* values
   */
  void ColorUtilities::getCIELabPCSFromSRGB(float &L, float &A, float &B, int r, int g, int b) {
    float X, Y, Z;
    ColorUtilities::getCIEXYZPCSFromSRGB(X, Y, Z, r, g, b);
    ColorUtilities::getCIELabFromXYZ(L, A, B, X, Y, Z);
  }

  /**
   * <p>Convert RGB values in sRGB to 16 bit fractional integer scaled CIELab in ICC PCS.</p>
   *
   * @param	rgb		array of length 3 containing R,G,B values each from 0 to 255
   * return			array of length 3 containing L*,a*,b* values each from 0 to 65535
   */
  void ColorUtilities::getIntegerScaledCIELabPCSFromSRGB(int &sL, int &sA, int &sB, int r, int g, int b) {
    float L, A, B;
    ColorUtilities::getCIELabPCSFromSRGB(L, A, B, r, g, b);
    ColorUtilities::getIntegerScaledCIELabFromCIELab(sL, sA, sB, L, A, B);
  }

  /**
   * <p>Convert CIELab in ICC PCS to RGB values in sRGB.</p>
   *
   * @param	cieLab	array of length 3 containing L*,a*,b* values
   * return			array of length 3 containing R,G,B values each from 0 to 255
   */
  void ColorUtilities::getSRGBFromCIELabPCS(int &r, int &g, int &b, float L, float A, float B) {
    float X, Y, Z;
    ColorUtilities::getCIEXYZFromLAB(X, Y, Z, L, A, B);
    ColorUtilities::getSRGBFromCIEXYZPCS(r, g, b, X, Y, Z);
  }

  /**
   * <p>Convert 16 bit fractional integer scaled CIELab in ICC PCS to RGB values in sRGB.</p>
   *
   * @param	cieLabScaled	array of length 3 containing L*,a*,b* values each from 0 to 65535
   * return					array of length 3 containing R,G,B values each from 0 to 255
   */
  void ColorUtilities::getSRGBFromIntegerScaledCIELabPCS(int &sr, int &sg, int &sb, int sL, int sA, int sB) {
    float L, A, B;
    ColorUtilities::getCIELabPCSFromIntegerScaledCIELabPCS(L, A, B, sL, sA, sB);
    ColorUtilities::getSRGBFromCIELabPCS(sr, sg, sb, L, A, B);
  }

}
