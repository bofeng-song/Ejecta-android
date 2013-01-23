#include "EJApp.h"
#include "EJBindingBase.h"
#include "EJCanvas/EJCanvasContext.h"
#include "EJCanvas/EJCanvasContextScreen.h"
#include "EJTimer.h"
#include "NSObjectFactory.h"

JSValueRef ej_global_undefined;
JSClassRef ej_constructorClass;

JSValueRef ej_getNativeClass(JSContextRef ctx, JSObjectRef object, JSStringRef propertyNameJS, JSValueRef* exception) {
 	size_t classNameSize = JSStringGetMaximumUTF8CStringSize(propertyNameJS);
    char* className = (char*)malloc(classNameSize);
    JSStringGetUTF8CString(propertyNameJS, className, classNameSize);
	
 	JSObjectRef obj = NULL;
 	NSString * fullClassName = new NSString();

    NSLOG("ej_getNativeClass : EJBinding%s", className);

 	fullClassName->initWithFormat("EJBinding%s",className);
 	EJBindingBase* pClass = (EJBindingBase*)NSClassFromString(fullClassName->getCString());
	if( pClass ) {
		obj = JSObjectMake( ctx, ej_constructorClass, (void *)pClass );
	} else {
		 NSLOG("%s is NULL ... ", fullClassName->getCString());
	}

    if (obj)
    {
    	NSLOG("constructor js-obj for %s", className);
    }
	
    free(className);
    fullClassName->autorelease();
 	return obj ? obj : ej_global_undefined;
 }

JSObjectRef ej_callAsConstructor(JSContextRef ctx, JSObjectRef constructor, size_t argc, const JSValueRef argv[], JSValueRef* exception) {
	


	EJBindingBase* pClass = (EJBindingBase*)(JSObjectGetPrivate( constructor ));
	
    NSLOG("ej_callAsConstructor constructor: %s()", pClass->toString().c_str());

	JSClassRef jsClass = EJApp::instance()->getJSClassForClass(pClass);

	JSObjectRef obj = JSObjectMake( ctx, jsClass, NULL );
	
 	EJBindingBase* instance = (EJBindingBase*)NSClassFromString(pClass->toString().c_str());
	instance->init(ctx, obj, argc, argv);

    NSLOG("binding constructor: %s", instance->toString().c_str());

	JSObjectSetPrivate( obj, (void *)instance );
	
 	return obj;
 }




// ---------------------------------------------------------------------------------
// Ejecta Main Class implementation - this creates the JavaScript Context and loads
// the initial JavaScript source files

EJApp* EJApp::ejectaInstance = NULL;


EJApp::EJApp() : currentRenderingContext(0), screenRenderingContext(0)
{
	landscapeMode = true;

	// Show the loading screen - commented out for now.
	// This causes some visual quirks on different devices, as the launch screen may be a 
	// different one than we loade here - let's rather show a black screen for 200ms...
	//NSString * loadingScreenName = [EJApp landscapeMode] ? @"Default-Landscape.png" : @"Default-Portrait.png";
	//loadingScreen = [[UIImageView alloc] initWithImage:[UIImage imageNamed:loadingScreenName]];
	//loadingScreen.frame = self.view.bounds;
	//[self.view addSubview:loadingScreen];
	
	paused = false;
	internalScaling = 1.0f;
	
	mainBundle = 0;
	
	timers = new EJTimerCollection();

	// Create the global JS context and attach the 'Ejecta' object
		jsClasses = NSDictionary::create();
		
		JSClassDefinition constructorClassDef = kJSClassDefinitionEmpty;
		constructorClassDef.callAsConstructor = ej_callAsConstructor;
		ej_constructorClass = JSClassCreate(&constructorClassDef);
		
		JSClassDefinition globalClassDef = kJSClassDefinitionEmpty;
		globalClassDef.getProperty = ej_getNativeClass;		
		JSClassRef globalClass = JSClassCreate(&globalClassDef);
		
		
		jsGlobalContext = JSGlobalContextCreate(NULL);
		ej_global_undefined = JSValueMakeUndefined(jsGlobalContext);
		JSValueProtect(jsGlobalContext, ej_global_undefined);
		JSObjectRef globalObject = JSContextGetGlobalObject(jsGlobalContext);
		
		JSObjectRef iosObject = JSObjectMake( jsGlobalContext, globalClass, NULL );
		JSObjectSetProperty(
			jsGlobalContext, globalObject, 
			JSStringCreateWithUTF8CString("Ejecta"), iosObject, 
			kJSPropertyAttributeDontDelete | kJSPropertyAttributeReadOnly, NULL
		);
		
		// Create the OpenGL ES1 Context
		// Android init GLView on java framework

		
	NSObjectFactory::map_type* base = NSObjectFactory::getMap();
	for(NSObjectFactory::map_type::iterator it = base->begin(); it != base->end(); it++)
	{
		string name = it->first;
		NSLOG("NSObjectFactory : %s", name.c_str());
	} 
		
	NSObjectFactory::fuc_map_type* fuc_base = NSObjectFactory::getFunctionMap();
	for(NSObjectFactory::fuc_map_type::iterator it = fuc_base->begin(); it != fuc_base->end(); it++)
	{
		string name = it->first;
		NSLOG("NSObjectFactory : %s", name.c_str());
	} 
		
}


EJApp::~EJApp()
{
	JSGlobalContextRelease(jsGlobalContext);
	currentRenderingContext->release();
	//[touchDelegate release];
	jsClasses->release();
	
	timers->release();
	if(mainBundle)
		free(mainBundle);
}

void EJApp::init(const char* path, int w, int h)
{

	if(mainBundle)
		free(mainBundle);

    int len = (strlen(path) + 1);
    mainBundle = (char *)malloc(len * sizeof(char));
    memset(mainBundle, 0, len);
    snprintf(mainBundle, len, "%s", path);

	height = h;
	width = w;

	// Load the initial JavaScript source files
	// loadScriptAtPath(NSStringMake(EJECTA_BOOT_JS));
	loadScriptAtPath(NSStringMake(EJECTA_MAIN_JS));
}


void EJApp::setScreenSize(int w, int h)
{
	height = h;
	width = w;
	if (screenRenderingContext)
	{
		// Redraw the canvas
		currentRenderingContext = (EJCanvasContext *)screenRenderingContext;
		
	}

	screenRenderingContext->prepare();
}

void EJApp::run(void)
{

	if( paused ) { return; }

	// Check all timers
	timers->update();
	
	if (screenRenderingContext)
	{
	// Redraw the canvas
		currentRenderingContext = (EJCanvasContext *)screenRenderingContext;
		
	}

	currentRenderingContext->flushBuffers();
}

void EJApp::pause(void)
{

	screenRenderingContext->finish();
	paused = true;
}

void EJApp::resume(void)
{
	paused = false;
}

void EJApp::clearCaches(void)
{
	JSGarbageCollect(jsGlobalContext);
}

void EJApp::hideLoadingScreen(void)
{
	//[loadingScreen removeFromSuperview];
	//[loadingScreen release];
	//loadingScreen = nil;
}

NSString * EJApp::pathForResource(NSString * resourcePath)
{
 	string full_path = string(mainBundle) + string("/") + string(EJECTA_APP_FOLDER) + resourcePath->getCString();
	return NSStringMake(full_path);
}

// ---------------------------------------------------------------------------------
// Script loading and execution

void EJApp::loadScriptAtPath(NSString * path)
{

	NSString * script = NSString::createWithContentsOfFile(pathForResource(path)->getCString());
	
	if( !script ) {
		NSLOG("Error: Can't Find Script %s", path->getCString() );
		return;
	}
	
	NSLOG("Loading Script: %s", path->getCString() );

	JSStringRef scriptJS = JSStringCreateWithUTF8CString(script->getCString());
	JSStringRef pathJS = JSStringCreateWithUTF8CString(path->getCString());
	
	JSValueRef exception = NULL;
	JSEvaluateScript( jsGlobalContext, scriptJS, NULL, pathJS, 0, &exception );
	logException(exception, jsGlobalContext);

	JSStringRelease( scriptJS );
	JSStringRelease( pathJS );

}

JSValueRef EJApp::loadModuleWithId(NSString * moduleId, JSValueRef module, JSValueRef exports)
{
	NSString * path = NSStringMake(moduleId->getCString() + string(".js"));
	NSString * script = NSString::createWithContentsOfFile(pathForResource(path)->getCString());
	
	if( !script ) {
		NSLog("Error: Can't Find Module %s", moduleId->getCString() );
		return NULL;
	}
	
	NSLog("Loading Module: %s", moduleId->getCString() );
	
	JSStringRef scriptJS = JSStringCreateWithUTF8CString(script->getCString());
	JSStringRef pathJS = JSStringCreateWithUTF8CString(path->getCString());
	JSStringRef parameterNames[] = {
		JSStringCreateWithUTF8CString("module"),
		JSStringCreateWithUTF8CString("exports"),
	};
	
	JSValueRef exception = NULL;
	JSObjectRef func = JSObjectMakeFunction( jsGlobalContext, NULL, 2,  parameterNames, scriptJS, pathJS, 0, &exception );
	
	JSStringRelease( scriptJS );
	JSStringRelease( pathJS );
	JSStringRelease(parameterNames[0]);
	JSStringRelease(parameterNames[1]);
	
	if( exception ) {
		logException(exception, jsGlobalContext);
		return NULL;
	}
	
	JSValueRef params[] = { module, exports };
	return invokeCallback(func, NULL, 2, params);
}

JSValueRef EJApp::invokeCallback(JSObjectRef callback, JSObjectRef thisObject, size_t argc, const JSValueRef argv[])
{
	JSValueRef exception = NULL;
	JSValueRef result = JSObjectCallAsFunction( jsGlobalContext, callback, thisObject, argc, argv, &exception );
	logException(exception,jsGlobalContext);
	return result;
}

//classId is EJBindingBase* or child class
JSClassRef EJApp::getJSClassForClass(EJBindingBase* classId)
{
	JSClassRef jsClass = NULL;

	if (jsClasses->count())
	{

		NSDictElement* pElement = NULL;
		NSObject* pObject = NULL;
		NSDICT_FOREACH(jsClasses, pElement)
		{
			string key = string(pElement->getStrKey());
	        if( key == classId->toString() ) {
				jsClass = (JSClassRef)((NSValue*)jsClasses->objectForKey(classId->toString()))->pointerValue();
				break;
			}	
		}
	}
	// Not already loaded? Ask the objc class for the JSClassRef!
	if( !jsClass ) {
		jsClass = classId->getJSClass(classId);
		jsClasses->setObject(new NSValue(jsClass, kJSClassRef), classId->toString());
	}
	return jsClass;
}

void EJApp::logException(JSValueRef valueAsexception, JSContextRef ctxp)
{
	if( !valueAsexception ) return;
	
	JSStringRef jsLinePropertyName = JSStringCreateWithUTF8CString("line");
	JSStringRef jsFilePropertyName = JSStringCreateWithUTF8CString("sourceURL");
	
	JSObjectRef exObject = JSValueToObject( ctxp, valueAsexception, NULL );
	JSValueRef valueAsline = JSObjectGetProperty( ctxp, exObject, jsLinePropertyName, NULL );
	JSValueRef valueAsfile = JSObjectGetProperty( ctxp, exObject, jsFilePropertyName, NULL );
	

    JSStringRef jsexception = JSValueToStringCopy(ctxp, valueAsexception, NULL);
    JSStringRef jsline = JSValueToStringCopy(ctxp, valueAsline, NULL);
    JSStringRef jsfile = JSValueToStringCopy(ctxp, valueAsfile, NULL);

    size_t jsexceptionSize = JSStringGetMaximumUTF8CStringSize(jsexception);
    char* exception = (char*)malloc(jsexceptionSize);
    JSStringGetUTF8CString(jsexception, exception, jsexceptionSize);

    size_t jslineSize = JSStringGetMaximumUTF8CStringSize(jsline);
    char* line = (char*)malloc(jslineSize);
    JSStringGetUTF8CString(jsline, line, jslineSize);

    size_t jsfileSize = JSStringGetMaximumUTF8CStringSize(jsfile);
    char* file = (char*)malloc(jsfileSize);
    JSStringGetUTF8CString(jsfile, file, jsfileSize);

	NSLog("%s at line %s in %s", exception, line, file);
	

    free(exception);
    free(line);
    free(file);
    JSStringRelease(jsexception);
    JSStringRelease(jsline);
    JSStringRelease(jsfile);

	JSStringRelease( jsLinePropertyName );
	JSStringRelease( jsFilePropertyName );
}


JSValueRef EJApp::createTimer(JSContextRef ctxp, size_t argc, const JSValueRef argv[],  BOOL repeat)
{
	if( argc != 2 || !JSValueIsObject(ctxp, argv[0]) || !JSValueIsNumber(jsGlobalContext, argv[1]) ) {
		return NULL;
	}
	
	JSObjectRef func = JSValueToObject(ctxp, argv[0], NULL);
	float interval = JSValueToNumber(ctxp, argv[1], NULL)/1000.0;
	
	// Make sure short intervals (< 18ms) run each frame
	if( interval < 0.018 ) {
		interval = 0;
	}
	
	int timerId = timers->scheduleCallback(func, interval, repeat);
	return JSValueMakeNumber( ctxp, timerId );
}

JSValueRef EJApp::deleteTimer(JSContextRef ctxp, size_t argc, const JSValueRef argv[])
{
	if( argc != 1 || !JSValueIsNumber(ctxp, argv[0]) ) return NULL;
	
	timers->cancelId(JSValueToNumber(ctxp, argv[0], NULL));
	return NULL;
}

void EJApp::setCurrentRenderingContext(EJCanvasContext * renderingContext)
{
	if( renderingContext != currentRenderingContext ) {
		currentRenderingContext->flushBuffers();
		currentRenderingContext->release();
		renderingContext->prepare();
		renderingContext->retain();
		currentRenderingContext = renderingContext;
	}
}

EJApp* EJApp::instance()
{
	if (ejectaInstance == NULL)
	{
		ejectaInstance = new EJApp();
	}
	return ejectaInstance;
}


void EJApp::finalize()
{
	if (ejectaInstance != NULL)
	{
		delete ejectaInstance;
		ejectaInstance = NULL;
	}
}