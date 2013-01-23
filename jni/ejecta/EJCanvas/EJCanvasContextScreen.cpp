#include "EJCanvasContextScreen.h"
#include "EJApp.h"


EJCanvasContextScreen::EJCanvasContextScreen()
{

}


EJCanvasContextScreen::EJCanvasContextScreen(short widthp, short heightp) : EJCanvasContext( widthp, heightp)
{

}

EJCanvasContextScreen::~EJCanvasContextScreen()
{

}

void EJCanvasContextScreen::present()
{
	// [self flushBuffers];
	EJCanvasContext::flushBuffers();
	
	if( msaaEnabled ) {
		//Bind the MSAA and View frameBuffers and resolve
		// glBindFramebuffer(GL_READ_FRAMEBUFFER_APPLE, msaaFrameBuffer);
		// glBindFramebuffer(GL_DRAW_FRAMEBUFFER_APPLE, viewFrameBuffer);
		// glResolveMultisampleFramebufferAPPLE();
		
		glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderBuffer);
		// [[EJApp instance].glContext presentRenderbuffer:GL_RENDERBUFFER];
		// EJApp::instance()->glContext->presentRenderbuffer(GL_RENDERBUFFER_OES);
		// presentRenderbuffer(GL_RENDERBUFFER_OES);
		glBindFramebufferOES(GL_FRAMEBUFFER_OES, msaaFrameBuffer);
	}
	else {
		// [[EJApp instance].glContext presentRenderbuffer:GL_RENDERBUFFER];
	}	
}

void EJCanvasContextScreen::finish()
{
	glFinish();	
}

void EJCanvasContextScreen::create()
{

	int m_width = EJApp::instance()->width;
	int m_height = EJApp::instance()->height;

	CGSize screen = {m_width, m_height};
	CGRect frame = {0, 0, screen};
// 	CGSize screen = [EJApp instance].view.bounds.size;
    float contentScale = 1;
    // useRetinaResolution
	float aspect = (float)frame.size.width / (float)frame.size.height;

	if( scalingMode == kEJScalingModeFitWidth ) {
		frame.size.width = screen.width;
		frame.size.height = screen.width / aspect;
	}
	else if( scalingMode == kEJScalingModeFitHeight ) {
		frame.size.width = screen.height * aspect;
		frame.size.height = screen.height;
	}
	float internalScaling = frame.size.width / (float)width;
	EJApp::instance()->internalScaling = internalScaling;
	
    backingStoreRatio = internalScaling * contentScale;
	
	bufferWidth = viewportWidth = frame.size.width * contentScale;
	bufferHeight = viewportHeight = frame.size.height * contentScale;
	
	NSLOG(
		"Creating ScreenCanvas: \nsize: %dx%d, aspect ratio: %.3f, \nscaled: %.3f = %dx%d, \nretina: no = %.0fx%.0f, \nmsaa: no",
		width, height, aspect, 
		internalScaling, frame.size.width, frame.size.height,
		frame.size.width * contentScale, frame.size.height * contentScale
	);
	
// 	// Create the OpenGL UIView with final screen size and content scaling (retina)
// 	glview = [[EAGLView alloc] initWithFrame:frame contentScale:contentScale];
	
// 	// This creates the frame- and renderbuffers
// 	[super create];
	EJCanvasContext::create();
	
// 	// Set up the renderbuffer and some initial OpenGL properties
// 	[[EJApp instance].glContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer *)glview.layer];
// 	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, viewRenderBuffer);
	

	glDisable(GL_CULL_FACE);
	glDisable(GL_LIGHTING);
	glDisable(GL_DITHER);
	
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_ALWAYS);

	prepare();
	
	glClearColor(0.0f, 0.0f, 1.0f, 0.5f);
    glClear(GL_COLOR_BUFFER_BIT);

// 	// Append the OpenGL view to Impact's main view
    EJApp::instance()->hideLoadingScreen();
}

void EJCanvasContextScreen::prepare()
{

	NSLOG("EJCanvasContextScreen prepare");
	
	EJCanvasContext::prepare();
	glTranslatef(0, height, 0);
	glScalef( 1, -1, 1 );
}

EJImageData* EJCanvasContextScreen::getImageData(float sx, float sy, float sw, float sh)
{
	if(backingStoreRatio != 1 && EJTexture::smoothScaling()) {
		NSLOG(
			"Warning: The screen canvas has been scaled; getImageData() may not work as expected. \n%s",
			"Set imageSmoothingEnabled=false or use an off-screen Canvas for more accurate results."
		);
	}
	
	// [self flushBuffers];
	flushBuffers();
	
	// Read pixels; take care of the upside down screen layout and the backingStoreRatio
	int internalWidth = sw * backingStoreRatio;
	int internalHeight = sh * backingStoreRatio;
	int internalX = sx * backingStoreRatio;
	int internalY = (height-sy-sh) * backingStoreRatio;
	
	EJColorRGBA * internalPixels = (EJColorRGBA*)malloc( internalWidth * internalHeight * sizeof(EJColorRGBA));
	glReadPixels( internalX, internalY, internalWidth, internalHeight, GL_RGBA, GL_UNSIGNED_BYTE, internalPixels );

	GLubyte * pixels = (GLubyte*)malloc( sw * sh * sizeof(GLubyte) * 4);
	int index = 0;
	for( int y = 0; y < sh; y++ ) {
		for( int x = 0; x < sw; x++ ) {
			int internalIndex = (int)((sh-y-1) * backingStoreRatio) * internalWidth + (int)(x * backingStoreRatio);
			pixels[ index *4 + 0 ] = (GLubyte)internalPixels[ internalIndex ].rgba.r;
			pixels[ index *4 + 1 ] = (GLubyte)internalPixels[ internalIndex ].rgba.g;
			pixels[ index *4 + 2 ] = (GLubyte)internalPixels[ internalIndex ].rgba.b;
			pixels[ index *4 + 3 ] = (GLubyte)internalPixels[ internalIndex ].rgba.a;
			index++;
		}
	}
	free(internalPixels);
	
	EJImageData* m_EJImageDate = new EJImageData(sw, sh, pixels);
	m_EJImageDate->autorelease();
	return m_EJImageDate;
}