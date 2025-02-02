/*
 * Stellarium
 * Copyright (C) 2008 Fabien Chereau
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef _STELPAINTER_HPP_
#define _STELPAINTER_HPP_
#include "VecMath.hpp"
#include "StelSphereGeometry.hpp"
#include "StelProjectorType.hpp"
#include "StelProjector.hpp"
#include <QString>
#include <QVarLengthArray>
#include <QFontMetrics>
#include <QOpenGLFunctions>

class QOpenGLShaderProgram;

class StelPainterLight
{
public:
	StelPainterLight(int alight=0) : light(alight), enabled(false) {}

	void setPosition(const Vec4f& v);
	Vec4f& getPosition() {return position;}

	void setDiffuse(const Vec4f& v);
	Vec4f& getDiffuse() {return diffuse;}

	void setSpecular(const Vec4f& v);
	Vec4f& getSpecular() {return specular;}

	void setAmbient(const Vec4f& v);
	Vec4f& getAmbient() {return ambient;}

	void setEnable(bool v);
	void enable();
	void disable();
	bool isEnabled() const {return enabled;}

private:
	int light;
	Vec4f position;
	Vec4f diffuse;
	Vec4f specular;
	Vec4f ambient;
	bool enabled;
};


//! @class StelPainter
//! Provides functions for performing openGL drawing operations.
//! All coordinates are converted using the StelProjector instance passed at construction.
//! Because openGL is not thread safe, only one instance of StelPainter can exist at a time, enforcing thread safety.
//! As a coding rule, no openGL calls should be performed when no instance of StelPainter exist.
//! Typical usage is to create a local instance of StelPainter where drawing operations are needed.
class StelPainter: protected QOpenGLFunctions
{
public:
	friend class VertexArrayProjector;

	//! Define the drawing mode when drawing polygons
	enum SphericalPolygonDrawMode
	{
		SphericalPolygonDrawModeFill=0,			//!< Draw the interior of the polygon
		SphericalPolygonDrawModeBoundary=1,		//!< Draw the boundary of the polygon
		SphericalPolygonDrawModeTextureFill=2	//!< Draw the interior of the polygon filled with the current texture
	};

	//! Define the drawing mode when drawing vertex
	enum DrawingMode
	{
		Points                      = 0x0000, //!< GL_POINTS
		Lines                       = 0x0001, //!< GL_LINES
		LineLoop                    = 0x0002, //!< GL_LINE_LOOP
		LineStrip                   = 0x0003, //!< GL_LINE_STRIP
		Triangles                   = 0x0004, //!< GL_TRIANGLES
		TriangleStrip               = 0x0005, //!< GL_TRIANGLE_STRIP
		TriangleFan                 = 0x0006  //!< GL_TRIANGLE_FAN
	};

	explicit StelPainter(const StelProjectorP& prj);
	~StelPainter();

	//! Return the instance of projector associated to this painter
	const StelProjectorP& getProjector() const {return prj;}
	void setProjector(const StelProjectorP& p);

	//! Fill with black around the viewport.
	void drawViewportShape();

	//! Draw the string at the given position and angle with the given font.
	//! If the gravity label flag is set, uses drawTextGravity180.
	//! @param x horizontal position of the lower left corner of the first character of the text in pixel.
	//! @param y horizontal position of the lower left corner of the first character of the text in pixel.
	//! @param str the text to print.
	//! @param angleDeg rotation angle in degree. Rotation is around x,y.
	//! @param xshift shift in pixel in the rotated x direction.
	//! @param yshift shift in pixel in the rotated y direction.
	//! @param noGravity don't take into account the fact that the text should be written with gravity.
	//! @param v direction vector of object to draw. GZ20120826: Will draw only if this is in the visible hemisphere.
	void drawText(float x, float y, const QString& str, float angleDeg=0.f,
			  float xshift=0.f, float yshift=0.f, const bool noGravity=true);
	void drawText(const Vec3d& v, const QString& str, const float angleDeg=0.f,
			  const float xshift=0.f, const float yshift=0.f, const bool noGravity=true);

	//! Draw the given SphericalRegion.
	//! @param region The SphericalRegion to draw.
	//! @param drawMode define whether to draw the outline or the fill or both.
	//! @param clippingCap if not set to NULL, tells the painter to try to clip part of the region outside the cap.
	//! @param doSubDivise if true tesselates the object to follow projection distortions.
	//! Typically set that to false if you think that the region is fully contained in the viewport.
	void drawSphericalRegion(const SphericalRegion* region, SphericalPolygonDrawMode drawMode=SphericalPolygonDrawModeFill, const SphericalCap* clippingCap=NULL, bool doSubDivise=true, double maxSqDistortion=5.);

	void drawGreatCircleArcs(const StelVertexArray& va, const SphericalCap* clippingCap=NULL);

	void drawSphericalTriangles(const StelVertexArray& va, const bool textured, const SphericalCap* clippingCap=NULL, const bool doSubDivide=true, const double maxSqDistortion=5.);

	//! Draw a small circle arc between points start and stop with rotation point in rotCenter.
	//! The angle between start and stop must be < 180 deg.
	//! The algorithm ensures that the line will look smooth, even for non linear distortion.
	//! Each time the small circle crosses the edge of the viewport, the viewportEdgeIntersectCallback is called with the
	//! screen 2d position, direction of the currently drawn arc toward the inside of the viewport.
	//! If rotCenter is equal to 0,0,0, the method draws a great circle.
	void drawSmallCircleArc(const Vec3d& start, const Vec3d& stop, const Vec3d& rotCenter, void (*viewportEdgeIntersectCallback)(const Vec3d& screenPos, const Vec3d& direction, void* userData)=NULL, void* userData=NULL);

	//! Draw a great circle arc between points start and stop.
	//! The angle between start and stop must be < 180 deg.
	//! The algorithm ensures that the line will look smooth, even for non linear distortion.
	//! Each time the small circle crosses the edge of the viewport, the viewportEdgeIntersectCallback is called with the
	//! screen 2d position, direction of the currently drawn arc toward the inside of the viewport.
	//! @param clippingCap if not set to NULL, tells the painter to try to clip part of the region outside the cap.
	void drawGreatCircleArc(const Vec3d& start, const Vec3d& stop, const SphericalCap* clippingCap=NULL, void (*viewportEdgeIntersectCallback)(const Vec3d& screenPos, const Vec3d& direction, void* userData)=NULL, void* userData=NULL);

	//! Draw a simple circle, 2d viewport coordinates in pixel
	void drawCircle(const float x, const float y, float r);

	//! Draw a square using the current texture at the given projected 2d position.
	//! This method is not thread safe.
	//! @param x x position in the viewport in pixel.
	//! @param y y position in the viewport in pixel.
	//! @param radius the half size of a square side in pixel.
	//! @param v direction vector of object to draw. GZ20120826: Will draw only if this is in the visible hemisphere.
	void drawSprite2dMode(const float x, const float y, float radius);
	void drawSprite2dMode(const Vec3d& v, const float radius);

	//! Same as drawSprite2dMode but don't scale according to display device scaling. 
	void drawSprite2dModeNoDeviceScale(const float x, const float y, const float radius);
	
	//! Draw a rotated square using the current texture at the given projected 2d position.
	//! This method is not thread safe.
	//! @param x x position in the viewport in pixel.
	//! @param y y position in the viewport in pixel.
	//! @param radius the half size of a square side in pixel.
	//! @param rotation rotation angle in degree.
	void drawSprite2dMode(float x, float y, float radius, float rotation);

	//! Draw a GL_POINT at the given position.
	//! @param x x position in the viewport in pixels.
	//! @param y y position in the viewport in pixels.
	void drawPoint2d(const float x, const float y);

	//! Draw a line between the 2 points.
	//! @param x1 x position of point 1 in the viewport in pixels.
	//! @param y1 y position of point 1 in the viewport in pixels.
	//! @param x2 x position of point 2 in the viewport in pixels.
	//! @param y2 y position of point 2 in the viewport in pixels.
	void drawLine2d(const float x1, const float y1, const float x2, const float y2);

	//! Draw a rectangle using the current texture at the given projected 2d position.
	//! This method is not thread safe.
	//! @param x x position of the top left corner in the viewport in pixel.
	//! @param y y position of the tope left corner in the viewport in pixel.
	//! @param width width in pixel.
	//! @param height height in pixel.
	//! @param textured whether the current texture should be used for painting.
	void drawRect2d(const float x, const float y, const float width, const float height, const bool textured=true);

	//! Re-implementation of gluSphere : glu is overridden for non-standard projection.
	//! @param radius
	//! @param oneMinusOblateness
	//! @param slices: number of vertical segments ("meridian zones")
	//! @param stacks: number of horizontal segments ("latitude zones")
	//! @param orientInside: 1 to have normals point inside, e.g. for landscape horizons
	//! @param flipTexture: if texture should be mapped to inside of shere, e.g. landscape horizons.
	//! @param topAngle GZ: new parameter. An opening angle [radians] at top of the sphere. Useful if there is an empty
	//!        region around the top pole, like for a spherical equirectangular horizon panorama (@class SphericalLandscape).
	//!        Example: your horizon line (pano photo) goes up to 26 degrees altitude (highest mountains/trees):
	//!        topAngle = 64 degrees = 64*M_PI/180.0f
	//! @param bottomAngle GZ: new parameter. An opening angle [radians] at bottom of the sphere. Useful if there is an empty
	//!        region around the bottom pole, like for a spherical equirectangular horizon panorama (SphericalLandscape class).
	//!        Example: your light pollution image (pano photo) goes down to just -5 degrees altitude (lowest street lamps below you):
	//!        bottomAngle = 95 degrees = 95*M_PI/180.0f
	void sSphere(const float radius, const float oneMinusOblateness, const int slices, const int stacks, const int orientInside = 0, const bool flipTexture = false,
				 const float topAngle=0.0f, const float bottomAngle=M_PI);

	//! Generate a StelVertexArray for a sphere.
	static StelVertexArray computeSphereNoLight(const float radius, const float oneMinusOblateness, const int slices, const int stacks,
												const int orientInside = 0, const bool flipTexture = false);

	//! Re-implementation of gluCylinder : glu is overridden for non-standard projection.
	void sCylinder(const float radius, const float height, const int slices, const int orientInside = 0);

	//! Draw a disk with a special texturing mode having texture center at center of disk.
	//! The disk is made up of concentric circles with increasing refinement.
	//! The number of slices of the outmost circle is (innerFanSlices<<level).
	//! @param radius the radius of the disk.
	//! @param innerFanSlices the number of slices.
	//! @param level the number of concentric circles.
	//! @param vertexArr the vertex array in which the resulting vertices are returned.
	//! @param texCoordArr the vertex array in which the resulting texture coordinates are returned.
	static void computeFanDisk(float radius, int innerFanSlices, int level, QVector<double>& vertexArr, QVector<float>& texCoordArr);

	//! Draw a ring with a radial texturing.
	void sRing(const float rMin, const float rMax, int slices, const int stacks, const int orientInside);

	//! Draw a fisheye texture in a sphere.
	void sSphereMap(const float radius, const int slices, const int stacks, const float textureFov = 2.f*M_PI, const int orientInside = 0);

	//! Set the font to use for subsequent text drawing.
	void setFont(const QFont& font);

	//! Set the color to use for subsequent drawing.
	void setColor(float r, float g, float b, float a=1.f);

	//! Get the color currently used for drawing.
	Vec4f getColor() const;

	//! Get the light
	StelPainterLight& getLight() {return light;}

	//! Get the font metrics for the current font.
	QFontMetrics getFontMetrics() const;

	//! Create the OpenGL shaders programs used by the StelPainter.
	//! This method needs to be called once at init.
	static void initGLShaders();
	
	//! Delete the OpenGL shaders objects.
	//! This method needs to be called once before exit.
	static void deinitGLShaders();

	//! Set whether texturing is enabled.
	void enableTexture2d(const bool b);

	// Thoses methods should eventually be replaced by a single setVertexArray
	//! use instead of glVertexPointer
	void setVertexPointer(const int size, const int type, const void* pointer) {
		vertexArray.size = size; vertexArray.type = type; vertexArray.pointer = pointer;
	}

	//! use instead of glTexCoordPointer
	void setTexCoordPointer(const int size, const int type, const void* pointer)
	{
		texCoordArray.size = size; texCoordArray.type = type; texCoordArray.pointer = pointer;
	}

	//! use instead of glColorPointer
	void setColorPointer(const int size, const int type, const void* pointer)
	{
		colorArray.size = size; colorArray.type = type; colorArray.pointer = pointer;
	}

	//! use instead of glNormalPointer
	void setNormalPointer(const int type, const void* pointer)
	{
		normalArray.size = 3; normalArray.type = type; normalArray.pointer = pointer;
	}

	//! use instead of glEnableClient
	void enableClientStates(const bool vertex, const bool texture=false, const bool color=false, const bool normal=false);

	//! convenience method that enable and set all the given arrays.
	//! It is equivalent to calling enableClientState and set the array pointer for each arrays.
	void setArrays(const Vec3d* vertices, const Vec2f* texCoords=NULL, const Vec3f* colorArray=NULL, const Vec3f* normalArray=NULL);
	void setArrays(const Vec3f* vertices, const Vec2f* texCoords=NULL, const Vec3f* colorArray=NULL, const Vec3f* normalArray=NULL);

	//! Draws primitives using vertices from the arrays specified by setVertexArray().
	//! @param mode The type of primitive to draw.
	//! If @param indices is NULL, this operation will consume @param count values from the enabled arrays, starting at @param offset.
	//! Else it will consume @param count elements of @param indices, starting at @param offset, which are used to index into the
	//! enabled arrays.
	void drawFromArray(const DrawingMode mode, const int count, const int offset=0, const bool doProj=true, const unsigned short *indices=NULL);

	//! Draws the primitives defined in the StelVertexArray.
	//! @param checkDiscontinuity will check and suppress discontinuities if necessary.
	void drawStelVertexArray(const StelVertexArray& arr, const bool checkDiscontinuity=true);

	//! Link an opengl program and show a message in case of error or warnings.
	//! @return true if the link was successful.
	static bool linkProg(class QOpenGLShaderProgram* prog, const QString& name);

	//! Sets whether the special planet shader should be used.
	void usePlanetShader(bool use);

private:

	friend class StelTextureMgr;
	friend class StelTexture;

	//! RAII class used to store and restore the opengl state.
	//! to use it we just need to instanciate it at the beginning of a method that might change the state.
	class GLState : protected QOpenGLFunctions
	{
	public:
		GLState();
		~GLState();
	private:
		bool blend;
		int blendSrcRGB, blendDstRGB, blendSrcAlpha, blendDstAlpha;
	};

	//! Struct describing one opengl array
	typedef struct
	{
		int size;				// The number of coordinates per vertex.
		int type;				// The data type of each coordinate (GL_SHORT, GL_INT, GL_FLOAT, or GL_DOUBLE).
		const void* pointer;	// Pointer to the first coordinate of the first vertex in the array.
		bool enabled;			// Define whether the array is enabled or not.
	} ArrayDesc;

	//! Project an array using the current projection.
	//! @return a descriptor of the new array
	ArrayDesc projectArray(const ArrayDesc& array, int offset, int count, const unsigned short *indices=NULL);

	//! Converts an array from double to float.
	void convertArrayToFloat(StelPainter::ArrayDesc& array, int offset, int count, const unsigned short *indices=NULL);

	//! Project the passed triangle on the screen ensuring that it will look smooth, even for non linear distortion
	//! by splitting it into subtriangles. The resulting vertex arrays are appended to the passed out* ones.
	//! The size of each edge must be < 180 deg.
	//! @param vertices a pointer to an array of 3 vertices.
	//! @param edgeFlags a pointer to an array of 3 flags indicating whether the next segment is an edge.
	//! @param texturePos a pointer to an array of 3 texture coordinates, or NULL if the triangle should not be textured.
	void projectSphericalTriangle(const SphericalCap* clippingCap, const Vec3d* vertices, QVarLengthArray<Vec3f, 4096>* outVertices,
			const Vec2f* texturePos=NULL, QVarLengthArray<Vec2f, 4096>* outTexturePos=NULL, const double maxSqDistortion=5., const int nbI=0,
			const bool checkDisc1=true, const bool checkDisc2=true, const bool checkDisc3=true) const;

	void drawTextGravity180(float x, float y, const QString& str, const float xshift = 0, const float yshift = 0);

	// Used by the method below
	static QVector<Vec2f> smallCircleVertexArray;
	void drawSmallCircleVertexArray();

	//! The associated instance of projector
	StelProjectorP prj;

#ifndef NDEBUG
	//! Mutex allowing thread safety
	static class QMutex* globalMutex;
#endif

	//! The used for text drawing
	QFont currentFont;

	//! Whether the special planet shader is used.
	bool planetShader;

	Vec4f currentColor;
	bool texture2dEnabled;
	
	static QOpenGLShaderProgram* basicShaderProgram;
	struct BasicShaderVars {
		int projectionMatrix;
		int color;
		int vertex;
	};
	static BasicShaderVars basicShaderVars;

	static QOpenGLShaderProgram* colorShaderProgram;
	static BasicShaderVars colorShaderVars;

	static QOpenGLShaderProgram* texturesShaderProgram;
	struct TexturesShaderVars {
		int projectionMatrix;
		int texCoord;
		int vertex;
		int texColor;
		int texture;
	};
	static TexturesShaderVars texturesShaderVars;
	static QOpenGLShaderProgram* texturesColorShaderProgram;
	struct TexturesColorShaderVars {
		int projectionMatrix;
		int texCoord;
		int vertex;
		int color;
		int texture;
	};
	static TexturesColorShaderVars texturesColorShaderVars;


	//! The descriptor for the current opengl vertex array
	ArrayDesc vertexArray;
	//! The descriptor for the current opengl texture coordinate array
	ArrayDesc texCoordArray;
	//! The descriptor for the current opengl normal array
	ArrayDesc normalArray;
	//! The descriptor for the current opengl color array
	ArrayDesc colorArray;

	//! the single light used by the painter
	StelPainterLight light;
};

#endif // _STELPAINTER_HPP_

