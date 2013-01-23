#include <GLES/gl.h>
#include <GLES/glext.h>
#include "EJCanvasContext.h"

EJVertex CanvasVertexBuffer[EJ_CANVAS_VERTEX_BUFFER_SIZE];

EJCanvasContext::EJCanvasContext() : viewFrameBuffer(0), viewRenderBuffer(0), msaaFrameBuffer(0), msaaRenderBuffer(0), stencilBuffer(0)
{
	NSLOG("EJCanvasContext");
}

//返回类名
const char* EJCanvasContext::getClassName() {
	return "EJCanvasContext";
}

EJCanvasContext::EJCanvasContext(short widthp, short heightp) : viewFrameBuffer(0), viewRenderBuffer(0), msaaFrameBuffer(0), msaaRenderBuffer(0), stencilBuffer(0)
{
	NSLOG("EJCanvasContext : %d, %d", widthp, heightp);

	memset(stateStack, 0, sizeof(stateStack));
	stateIndex = 0;
	state = &stateStack[stateIndex];
	state->globalAlpha = 1;
	state->globalCompositeOperation = kEJCompositeOperationSourceOver;
	state->transform = CGAffineTransformIdentity;
	state->lineWidth = 1;
	state->lineCap = kEJLineCapButt;
	state->lineJoin = kEJLineJoinMiter;
	state->miterLimit = 10;
	state->textBaseline = kEJTextBaselineAlphabetic;
	state->textAlign = kEJTextAlignStart;
	//state->font = [[UIFont fontWithName:@"Helvetica" size:10] retain];
	state->clipPath = NULL;
	
	bufferWidth = viewportWidth = width = widthp;
	bufferHeight = viewportHeight = height = heightp;
	
	path = new EJPath();
	backingStoreRatio = 1;
	
	// fontCache = [[NSCache alloc] init];
	// fontCache.countLimit = 8;
	
	imageSmoothingEnabled = true;
	msaaEnabled = false;
	msaaSamples = 2;
}

EJCanvasContext::~EJCanvasContext()
{
	//[fontCache release];
	
	// Release all fonts and clip paths from the stack
	for( int i = 0; i < stateIndex + 1; i++ ) {
		//[stateStack[i].font release];
		stateStack[i].clipPath->release();
	}
	
	if( viewFrameBuffer ) { glDeleteFramebuffersOES( 1, &viewFrameBuffer); }
	if( viewRenderBuffer ) { glDeleteRenderbuffersOES(1, &viewRenderBuffer); }
	if( msaaFrameBuffer ) {	glDeleteFramebuffersOES( 1, &msaaFrameBuffer); }
	if( msaaRenderBuffer ) { glDeleteRenderbuffersOES(1, &msaaRenderBuffer); }
	if( stencilBuffer ) { glDeleteRenderbuffersOES(1, &stencilBuffer); }
	
	path->release();

}

void EJCanvasContext::create()
{

	if( msaaEnabled ) {
		glGenFramebuffersOES(1, &msaaFrameBuffer);
		glBindFramebufferOES(GL_FRAMEBUFFER_OES, msaaFrameBuffer);
		
		glGenRenderbuffersOES(1, &msaaRenderBuffer);
		glBindRenderbufferOES(GL_RENDERBUFFER_OES, msaaRenderBuffer);
		
		//glRenderbufferStorageMultisampleIMG(GL_RENDERBUFFER_OES, msaaSamples, GL_RGBA8_OES, bufferWidth, bufferHeight);
		glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, msaaRenderBuffer);
	}
	
	// glGenFramebuffersOES(1, &viewFrameBuffer);
	// glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFrameBuffer);
	
	// glGenRenderbuffersOES(1, &viewRenderBuffer);
	// glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderBuffer);

}

void EJCanvasContext::createStencilBufferOnce()
{
	if( stencilBuffer ) { return; }
	
	glGenRenderbuffersOES(1, &stencilBuffer);
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, stencilBuffer);
	if( msaaEnabled ) {
		//glRenderbufferStorageMultisampleAPPLE(GL_RENDERBUFFER, msaaSamples, GL_DEPTH24_STENCIL8_OES, bufferWidth, bufferHeight);
	}
	else {
		glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH24_STENCIL8_OES, bufferWidth, bufferHeight);
	}
	glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, stencilBuffer);
	glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_STENCIL_ATTACHMENT_OES, GL_RENDERBUFFER_OES, stencilBuffer);
	
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, msaaEnabled ? msaaRenderBuffer : viewRenderBuffer );
	
	glClear(GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

}

void EJCanvasContext::bindVertexBuffer()
{
	glVertexPointer(2, GL_FLOAT, sizeof(EJVertex), &CanvasVertexBuffer[0].pos.x);
	glTexCoordPointer(2, GL_FLOAT, sizeof(EJVertex), &CanvasVertexBuffer[0].uv.x);
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(EJVertex), &CanvasVertexBuffer[0].color);
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

}

void EJCanvasContext::prepare()
{
	NSLOG("EJCanvasContext prepare");
	// //Bind the frameBuffer and vertexBuffer array
	// glBindFramebufferOES(GL_FRAMEBUFFER_OES, msaaEnabled ? msaaFrameBuffer : viewFrameBuffer );
	// glBindRenderbufferOES(GL_RENDERBUFFER_OES, msaaEnabled ? msaaRenderBuffer : viewRenderBuffer );
	
	NSLOG("viewportWidth : %d, viewportHeight : %d",viewportWidth, viewportHeight);
	glViewport(0, 0, viewportWidth, viewportHeight);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrthof(0, width, 0, height, -1, 1);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	EJCompositeOperation op = state->globalCompositeOperation;
	glBlendFunc( EJCompositeOperationFuncs[op].source, EJCompositeOperationFuncs[op].destination );
	glDisable(GL_TEXTURE_2D);
	currentTexture = NULL;
	EJTexture::setSmoothScaling(imageSmoothingEnabled);
	
	bindVertexBuffer();
	
	if( state->clipPath ) {
		glDepthFunc(GL_EQUAL);
	}
	else {
		glDepthFunc(GL_ALWAYS);
	}
}

void EJCanvasContext::setTexture(EJTexture * newTexture)
{
	if( currentTexture == newTexture ) { return; }
	
	flushBuffers();
		
	if( !newTexture && currentTexture ) {
		// Was enabled; should be disabled
		glDisable(GL_TEXTURE_2D);
	}
	else if( newTexture && !currentTexture ) {
		// Was disabled; should be enabled
		glEnable(GL_TEXTURE_2D);
	}
	
	currentTexture = newTexture;
	currentTexture->bind();
}

void EJCanvasContext::pushTri(float x1, float y1, float x2, float y2, float x3, float y3, EJColorRGBA color, CGAffineTransform transform)
{
	if( vertexBufferIndex >= EJ_CANVAS_VERTEX_BUFFER_SIZE - 3 ) {
		flushBuffers();
	}
	
	EJVector2 d1 = { x1, y1 };
	EJVector2 d2 = { x2, y2 };
	EJVector2 d3 = { x3, y3 };
	
	if( !CGAffineTransformIsIdentity(transform) ) {
		d1 = EJVector2ApplyTransform( d1, transform );
		d2 = EJVector2ApplyTransform( d2, transform );
		d3 = EJVector2ApplyTransform( d3, transform );
	}
	
	EJVertex * vb = &CanvasVertexBuffer[vertexBufferIndex];
	vb[0] = (EJVertex) { d1, {0.5, 1}, color };
	vb[1] = (EJVertex) { d2, {0.5, 0.5}, color };
	vb[2] = (EJVertex) { d3, {0.5, 1}, color };
	
	vertexBufferIndex += 3;
}

void EJCanvasContext::pushQuad(EJVector2 v1, EJVector2 v2, EJVector2 v3, EJVector2 v4, EJVector2 t1, EJVector2 t2, EJVector2 t3, EJVector2 t4, EJColorRGBA color, CGAffineTransform transform)
{
	if( vertexBufferIndex >= EJ_CANVAS_VERTEX_BUFFER_SIZE - 6 ) {
		flushBuffers();
	}
	
	if( !CGAffineTransformIsIdentity(transform) ) {
		v1 = EJVector2ApplyTransform( v1, transform );
		v2 = EJVector2ApplyTransform( v2, transform );
		v3 = EJVector2ApplyTransform( v3, transform );
		v4 = EJVector2ApplyTransform( v4, transform );
	}
	
	EJVertex * vb = &CanvasVertexBuffer[vertexBufferIndex];
	vb[0] = (EJVertex) { v1, t1, color };
	vb[1] = (EJVertex) { v2, t2, color };
	vb[2] = (EJVertex) { v3, t3, color };
	vb[3] = (EJVertex) { v2, t2, color };
	vb[4] = (EJVertex) { v3, t3, color };
	vb[5] = (EJVertex) { v4, t4, color };
	
	vertexBufferIndex += 6;
}

void EJCanvasContext::pushRect(float x, float y, float w, float h, float tx, float ty, float tw, float th, EJColorRGBA color, CGAffineTransform transform)
{

	if( vertexBufferIndex >= EJ_CANVAS_VERTEX_BUFFER_SIZE - 6 ) {
		flushBuffers();
	}
	
	EJVector2 d11 = { x, y };
	EJVector2 d21 = { x+w, y };
	EJVector2 d12 = { x, y+h };
	EJVector2 d22 = { x+w, y+h };
	
	if( !CGAffineTransformIsIdentity(transform) ) {
		d11 = EJVector2ApplyTransform( d11, transform );
		d21 = EJVector2ApplyTransform( d21, transform );
		d12 = EJVector2ApplyTransform( d12, transform );
		d22 = EJVector2ApplyTransform( d22, transform );
	}
	
	EJVertex * vb = &CanvasVertexBuffer[vertexBufferIndex];
	vb[0] = (EJVertex) { d11, {tx, ty}, color };	// top left
	vb[1] = (EJVertex) { d21, {tx+tw, ty}, color };	// top right
	vb[2] = (EJVertex) { d12, {tx, ty+th}, color };	// bottom left
		
	vb[3] = (EJVertex) { d21, {tx+tw, ty}, color };	// top right
	vb[4] = (EJVertex) { d12, {tx, ty+th}, color };	// bottom left
	vb[5] = (EJVertex) { d22, {tx+tw, ty+th}, color };// bottom right
	
	vertexBufferIndex += 6;
}

void EJCanvasContext::flushBuffers()
{
	if( vertexBufferIndex == 0 ) { return; }

	glVertexPointer(2, GL_FLOAT, sizeof(EJVertex), &CanvasVertexBuffer[0].pos.x);

	glDrawArrays(GL_TRIANGLES, 0, vertexBufferIndex);
	vertexBufferIndex = 0;
}

void EJCanvasContext::save()
{
	if( stateIndex == EJ_CANVAS_STATE_STACK_SIZE-1 ) {
		NSLog("Warning: EJ_CANVAS_STATE_STACK_SIZE (%d) reached", EJ_CANVAS_STATE_STACK_SIZE);
		return;
	}
	
	stateStack[stateIndex+1] = stateStack[stateIndex];
	stateIndex++;
	state = &stateStack[stateIndex];
	//[state->font retain];
	state->clipPath->retain();
}

void EJCanvasContext::restore()
{
	if( stateIndex == 0 ) {
		NSLog("Warning: Can't pop stack at index 0");
		return;
	}
	
	EJCompositeOperation oldCompositeOp = state->globalCompositeOperation;
	EJPath * oldClipPath = state->clipPath;
	
	// Clean up current state
	//[state->font release];

	if( state->clipPath && state->clipPath != stateStack[stateIndex-1].clipPath ) {
		resetClip();
	}
	state->clipPath->release();
	
	// Load state from stack
	stateIndex--;
	state = &stateStack[stateIndex];
	
    path->transform = state->transform;
    
	// Set Composite op, if different
	if( state->globalCompositeOperation != oldCompositeOp ) {
		globalCompositeOperation = state->globalCompositeOperation;
	}
	
	// Render clip path, if present and different
	if( state->clipPath && state->clipPath != oldClipPath ) {
		state->clipPath->drawPolygonsToContext(this,kEJPathPolygonTargetDepth);
	}
}

void EJCanvasContext::rotate(float angle)
{
	state->transform = CGAffineTransformRotate( state->transform, angle );
    path->transform = state->transform;
}

void EJCanvasContext::translate(float x, float y)
{
	state->transform = CGAffineTransformTranslate( state->transform, x, y );
    path->transform = state->transform;
}

void EJCanvasContext::scale(float x, float y)
{
	state->transform = CGAffineTransformScale( state->transform, x, y );
	path->transform = state->transform;
}

void EJCanvasContext::transform(float m11, float m12, float m21, float m22, float dx, float dy)
{
	CGAffineTransform t = CGAffineTransformMake( m11, m12, m21, m22, dx, dy );
	state->transform = CGAffineTransformConcat( t, state->transform );
	path->transform = state->transform;
}

void EJCanvasContext::setTransform(float m11, float m12, float m21, float m22, float dx, float dy)
{
	state->transform = CGAffineTransformMake( m11, m12, m21, m22, dx, dy );
	path->transform = state->transform;
}

void EJCanvasContext::drawImage(EJTexture * texture, float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh)
{
	float tw = texture->realWidth;
	float th = texture->realHeight;
	
	//EJColorRGBA color = {{255, 255, 255, 255 * state->globalAlpha}};
	EJColorRGBA color = {0xffffffff};
	color.rgba.a = 255 * state->globalAlpha;
	setTexture(texture);
	pushRect(dx, dy, dw, dh, sx/tw, sy/th, sw/tw, sh/th, color, state->transform);
}

void EJCanvasContext::fillRect(float x, float y, float w, float h)
{
	setTexture(NULL);
	
	EJColorRGBA color = state->fillColor;
	color.rgba.a = (float)color.rgba.a * state->globalAlpha;
	pushRect(x, y, w, h, 0, 0, 0, 0, color, state->transform);
}

void EJCanvasContext::strokeRect(float x, float y, float w, float h)
{
	// strokeRect should not affect the current path, so we create
	// a new, tempPath instead.
	EJPath * tempPath = new EJPath();
	tempPath->transform = state->transform;
	
	tempPath->moveTo(x, y);
	tempPath->lineTo(x+w, y);
	tempPath->lineTo(x+w, y+h);
	tempPath->lineTo(x, y+h);
	tempPath->close();
	
	tempPath->drawLinesToContext(this);
	tempPath->release();

}

void EJCanvasContext::clearRect(float x, float y, float w, float h)
{
	setTexture(NULL);
	
	EJCompositeOperation oldOp = state->globalCompositeOperation;
	globalCompositeOperation = kEJCompositeOperationDestinationOut;
	
	static EJColorRGBA white = {0xffffffff};
	pushRect(x, y, w, h, 0, 0, 0, 0, white, state->transform);
	
	globalCompositeOperation = oldOp;

}

EJImageData* EJCanvasContext::getImageData(float sx, float sy, float sw, float sh)
{
	flushBuffers();
	GLubyte * pixels = (GLubyte*)malloc( sw * sh * 4 * sizeof(GLubyte));
	glReadPixels(sx, sy, sw, sh, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	EJImageData* imageData = new EJImageData(sw, sh, pixels);
	imageData->autorelease();
	return imageData;
}

void EJCanvasContext::putImageData(EJImageData* imageData, float dx, float dy)
{
	EJTexture * texture = imageData->m_texture;
	setTexture(texture);
	
	short tw = texture->realWidth;
	short th = texture->realHeight;
	
	static EJColorRGBA white = {0xffffffff};
	
	pushRect(dx, dy, tw, th, 0, 0, 1, 1, white, CGAffineTransformIdentity);
	flushBuffers();

}

void EJCanvasContext::beginPath()
{
	path->reset();
}

void EJCanvasContext::closePath()
{
	path->close();
}

void EJCanvasContext::fill()
{
	path->drawPolygonsToContext(this,  kEJPathPolygonTargetColor);
}

void EJCanvasContext::stroke()
{
	path->drawLinesToContext(this);
}

void EJCanvasContext::moveTo(float x, float y)
{
	path->moveTo(x, y);
}

void EJCanvasContext::lineTo(float x, float y)
{
	path->lineTo(x, y);
}

void EJCanvasContext::rect(float x, float y, float w, float h)
{
	path->moveTo(x, y);
	path->lineTo(x+w, y);
	path->lineTo(x+w, y+h);
	path->lineTo(x, y+h);
	path->close();
}

void EJCanvasContext::bezierCurveTo(float cpx, float cpy, float cpx2, float cpy2, float x, float y)
{
	float scale = CGAffineTransformGetScale( state->transform );
	path->quadraticCurveTo(cpx, cpy, x, y, scale);
}

void EJCanvasContext::quadraticCurveTo(float cpx, float cpy, float x, float y)
{
	float scale = CGAffineTransformGetScale( state->transform );
	path->quadraticCurveTo(cpx, cpy, x, y, scale);
}

void EJCanvasContext::arcTo(float x1, float y1, float x2, float y2, float radius)
{
	path->arcTo(x1, y1, x2, y2, radius);
}

void EJCanvasContext::arc(float x, float y, float radius, float startAngle, float endAngle, BOOL antiClockwise)
{
	path->arc(x, y, radius, startAngle, endAngle, antiClockwise);
}


void EJCanvasContext::fillText(NSString * text, float x, float y)
{
	// EJFont *font = [self acquireFont:state->font.fontName size:state->font.pointSize fill:YES contentScale:backingStoreRatio];
	// [font drawString:text toContext:self x:x y:y];
}

void EJCanvasContext::strokeText(NSString * text, float x, float y)
{
	// EJFont *font = [self acquireFont:state->font.fontName size:state->font.pointSize fill:NO contentScale:backingStoreRatio];
	// [font drawString:text toContext:self x:x y:y];
}

float EJCanvasContext::measureText(NSString * text)
{
	// EJFont *font = [self acquireFont:state->font.fontName size:state->font.pointSize fill:YES contentScale:backingStoreRatio];
	// return [font measureString:text];
	return 0.0f;
}

void EJCanvasContext::clip()
{
	flushBuffers();
	state->clipPath->release();
	state->clipPath = NULL;
	
	state->clipPath = (EJPath*)(path->copy());
	state->clipPath->drawPolygonsToContext(this, kEJPathPolygonTargetDepth);
}

void EJCanvasContext::resetClip()
{
	if( state->clipPath ) {
		flushBuffers();
		state->clipPath->release();
		state->clipPath = NULL;
		
		glDepthMask(GL_TRUE);
		glClear(GL_DEPTH_BUFFER_BIT);
		glDepthMask(GL_FALSE);
		glDepthFunc(GL_ALWAYS);
	}
}