#ifndef DCMQI_COLORUTILITIES_H
#define DCMQI_COLORUTILITIES_H


using namespace std;

namespace dcmqi {

class ColorUtilities {
public:

  static void getIntegerScaledCIELabFromCIELab(int &sL, int &sA, int &sB, float L, float A, float B);

  /**
   * <p>Convert 16 bit fractional integer scaled CIELab values used in ICC profiles and DICOM to floating point.</p>
   *
   * <p>See ICC v4.3 Tables 12 and 13, and DICOM PS 3.3 C.10.7.1.1.</p>
   *
   * @param	cieLabScaled	array of length 3 containing 16 bit scaled L*,a*,b* values from  0 to 65535
   * return					array of length 3 containing L*,a*,b* values with L* from 0.0 to 100.0, and a* and b* from -128.0 to +127.0
   */
  static void getCIELabPCSFromIntegerScaledCIELabPCS(float &L, float &A, float &B, int sL, int sA, int sB);

  /**
   * <p>Convert CIEXYZ to CIE 1976 L*, a*, b*.</p>
   *
   * @see <a href="http://en.wikipedia.org/wiki/Lab_color_space#CIELAB-CIEXYZ_conversions">Wikipedia CIELAB-CIEXYZ_conversions</a>
   *
   * @param	cieXYZ	array of length 3 containing X,Y,Z values
   * return			array of length 3 containing L*,a*,b* values
   */
  static void getCIELabFromXYZ(float &L, float &A, float &B, float X, float Y, float Z);

  /**
   * <p>Convert CIE 1976 L*, a*, b* to CIEXYZ.</p>
   *
   * @see <a href="http://en.wikipedia.org/wiki/Lab_color_space#CIELAB-CIEXYZ_conversions">Wikipedia CIELAB-CIEXYZ_conversions</a>
   *
   * @param	cieLab	array of length 3 containing L*,a*,b* values
   * return			array of length 3 containing X,Y,Z values
   */
  static void getCIEXYZFromLAB(float &X, float &Y, float &Z, float L, float A, float B);

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
  static void getCIEXYZPCSFromSRGB(float &X, float &Y, float &Z, int R, int G, int B);

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
  static void getSRGBFromCIEXYZPCS(int &R, int &G, int &B, float X, float Y, float Z);

  /**
   * <p>Convert RGB values in sRGB to CIELab in ICC PCS.</p>
   *
   * @param	rgb		array of length 3 containing R,G,B values each from 0 to 255
   * return			array of length 3 containing L*,a*,b* values
   */
  static void getCIELabPCSFromSRGB(float &L, float &A, float &B, int r, int g, int b);

  /**
   * <p>Convert RGB values in sRGB to 16 bit fractional integer scaled CIELab in ICC PCS.</p>
   *
   * @param	rgb		array of length 3 containing R,G,B values each from 0 to 255
   * return			array of length 3 containing L*,a*,b* values each from 0 to 65535
   */
  static void getIntegerScaledCIELabPCSFromSRGB(int &sL, int &sA, int &sB, int r, int g, int b);

  /**
   * <p>Convert CIELab in ICC PCS to RGB values in sRGB.</p>
   *
   * @param	cieLab	array of length 3 containing L*,a*,b* values
   * return			array of length 3 containing R,G,B values each from 0 to 255
   */
  static void getSRGBFromCIELabPCS(int &r, int &g, int &b, float L, float A, float B);

  /**
   * <p>Convert 16 bit fractional integer scaled CIELab in ICC PCS to RGB values in sRGB.</p>
   *
   * @param	cieLabScaled	array of length 3 containing L*,a*,b* values each from 0 to 65535
   * return					array of length 3 containing R,G,B values each from 0 to 255
   */
  static void getSRGBFromIntegerScaledCIELabPCS(int &sr, int &sg, int &sb, int sL, int sA, int sB);

};
}

#endif // DCMQI_COLORUTILITIES_H
