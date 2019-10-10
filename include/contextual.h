#include <iostream>
#include <functional>
#include <optional>
#include <exception>
/*

A context manager is a class the manages two entities:
	1. resources -- temporary changes that must be made to execute a certain section of code
	2. a context -- a code block that is to be executed with temporarily held resources

Resources and contexts are given their own classes in this implementation. The Resource class
is responsible for the knowledge of resource acquistion and release. The context is responsible
for knowing the code block to be exectued.

A context manager must then perform the following actions:
	1. Direct the Resource class to acquire resources
	2. Make the resources available to the code block owned by the Context class
	3. Execute the code owned by the context
	4. Perform appropriate exception handling if necessary
	5. Release the resources and cleanup before exiting back to outer scope.


A class With type is defined so that its brace-initialization resembles a traditional
code block and gives the impression of With being a keyword. Most of its rule of five
operations are deleted to enforce this usage as it is not a traditional object.


*/

namespace Contextual {

// Forward declaration of the With class
class With;
// The struct that will hold the resources acquired for the context
struct Data;

/********************************************
*											*
* 	Basic struct to hold the code block 	*
*	to be executed with acquired resources	*
*											*
********************************************/


struct Context {
private:
	std::function<void(Data*)> code_block;
public:
	friend class With;
	Context(std::function<void(Data*)> code_block): code_block(std::move(code_block)){};
	
};

/********************************************
*											*
* 	The resource manager class interface	*
*											*
********************************************/

class IResource {
private:
	// We store the context so that it is not deallocated prematurely
	std::optional<Context> ctxt = std::nullopt;
	
protected:
	// The actual resources
	Data* resources;
	virtual void enter() = 0;
	virtual void exit(std::optional<std::exception> e) = 0;

public:
	friend class With;
	
	IResource() = default;
	IResource(Data* resources) : resources(resources){};
	With operator+(const Context& context);
	
};

/************************************
*									*
* 	The With class the emulates a 	*
*        keyword with code block 	*
*									*
************************************/

class With {
private:
	Context* _context = nullptr;

	void _run(){
		try{
			// Execute the context
			resource->enter();
			_context->code_block(resource->resources);
			
		} catch (std::exception& e) {
			// cleanup
			resource->exit(e);
			return;
		}
		resource->exit(std::nullopt);
	}

public:
	IResource* resource=nullptr;
	// The rule of five
	With() = delete;
	With(const With& other) = delete;
	With& operator=(const With& other) = delete;
	With& operator=(const With&& other) = delete;

	~With() = default;

	With(IResource* resource, Context* context): resource(resource),
													_context(context) {
														_run();
													};

};

//********************************************************

With IResource::operator+(const Context& context){
	ctxt = context;
	return With(this, &ctxt.value());

}

};

/********************************************************
	Example usage
*********************************************************
	With {
		Resource(Basedata struct) + Context{
	
			[&](auto resource){
				...code
			}
		}
	};

*********************************************************/