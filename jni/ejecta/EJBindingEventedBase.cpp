#include "EJBindingEventedBase.h"


EJBindingEventedBase::EJBindingEventedBase() {

	eventListeners =  new NSDictionary();
    if (eventListeners != NULL)
    {
        eventListeners->retain();
    }
	onCallbacks =  new NSDictionary();
    if (onCallbacks != NULL)
    {
        onCallbacks->retain();
    }

}

EJBindingEventedBase::EJBindingEventedBase(JSContextRef ctxp, JSObjectRef obj,
		size_t argc, const JSValueRef argv[]) {

	eventListeners =  new NSDictionary();
    if (eventListeners != NULL)
    {
        eventListeners->retain();
    }
	onCallbacks =  new NSDictionary();
    if (onCallbacks != NULL)
    {
        onCallbacks->retain();
    }
}

//
EJBindingEventedBase::~EJBindingEventedBase() {
	JSContextRef ctx = EJApp::instance()->jsGlobalContext;

	// Unprotect all event callbacks
	NSDictElement* eElement = NULL;
	NSDICT_FOREACH(eventListeners, eElement) {
		NSArray * listeners = (NSArray *) eElement->getObject()->copy();

		for (int var = 0; var < listeners->count(); ++var) {
			NSValue * callbackValue = (NSValue *) listeners->objectAtIndex(var);
			JSValueUnprotect(ctx, (JSValueRef) callbackValue->pointerValue());
		}
	}
	delete eventListeners;

	// Unprotect all event callbacks
	NSDictElement* oElement = NULL;
	NSDICT_FOREACH(eventListeners, oElement) {
		NSValue * listener = (NSValue *) oElement->getObject()->copy();
		JSValueUnprotect(ctx, (JSValueRef) listener->pointerValue());
	}
	delete onCallbacks;
}
//
JSObjectRef EJBindingEventedBase::getCallbackWith(NSString * name,
		JSContextRef ctx) {
	NSValue * listener = (NSValue *) onCallbacks->objectForKey(
			name->getCString());
	return listener ? (JSObjectRef) listener->pointerValue() : NULL;
}
//
void EJBindingEventedBase::setCallbackWith(NSString * name, JSContextRef ctx,
		JSValueRef callbackValue) {
	// remove old event listener?
	JSObjectRef oldCallback = getCallbackWith(name, ctx);
	if (oldCallback) {
		JSValueUnprotect(ctx, oldCallback);
		onCallbacks->removeObjectForKey(name->getCString());
	}

	JSObjectRef callback = JSValueToObject(ctx, callbackValue, NULL);
	if (callback && JSObjectIsFunction(ctx, callback)) {
		JSValueProtect(ctx, callback);
		onCallbacks->setObject(new NSValue(callback, kJSObjectRef), name->getCString());
		return;
	}
}
//
EJ_BIND_FUNCTION(EJBindingEventedBase,addEventListener, ctx, argc, argv) {
	if (argc < 2) {
		return NULL;
	}

	NSString * name = JSValueToNSString(ctx, argv[0]);
	JSObjectRef callback = JSValueToObject(ctx, argv[1], NULL);
	JSValueProtect(ctx, callback);
	NSValue * callbackValue = new NSValue(callback, kJSObjectRef);

	NSArray * listeners = NULL;
	if ((listeners = (NSArray *) eventListeners->objectForKey(
			name->getCString()))) {
		listeners->addObject(callbackValue);
	} else {
		eventListeners->setObject(NSArray::createWithObject(callbackValue),
				name->getCString());
	}
	return NULL;
}
//
EJ_BIND_FUNCTION(EJBindingEventedBase,removeEventListener, ctx, argc, argv) {
	if( argc < 2 ) { return NULL; }

	NSString * name = JSValueToNSString( ctx, argv[0] );

	NSArray * listeners = NULL;
	if( (listeners = (NSArray *)eventListeners->objectForKey(name->getCString()) ) ) {
		JSObjectRef callback = JSValueToObject(ctx, argv[1], NULL);
		for( int i = 0; i < listeners->count(); i++ ) {
			NSValue* temp =  (NSValue *)listeners->objectAtIndex(i);
			if( JSValueIsStrictEqual(ctx, callback, (JSObjectRef)temp->pointerValue() ) ) {
				listeners->removeObjectAtIndex(i);
				return NULL;
			}
		}
	}
	return NULL;
}
//
void EJBindingEventedBase::triggerEvent(NSString * name, int argc,
		JSValueRef argv[]) {
	EJApp * ejecta = EJApp::instance();

	NSArray * listeners = (NSArray *) eventListeners->objectForKey(
			name->getCString());
	if (listeners) {

		for (int var = 0; var < listeners->count(); ++var) {
			NSValue * callbackValue = (NSValue *) listeners->objectAtIndex(var);
			ejecta->invokeCallback((JSObjectRef)callbackValue->pointerValue(), jsObject,
					argc, argv);
		}
	}

	NSValue * callbackValue = (NSValue *) onCallbacks->objectForKey(
			name->getCString());
	if (callbackValue) {
		ejecta->invokeCallback((JSObjectRef)callbackValue->pointerValue(), jsObject, argc,
				argv);
	}
}

REFECTION_CLASS_IMPLEMENT(EJBindingEventedBase);