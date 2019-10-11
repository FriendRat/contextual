#include <contextual.h>
#include <stdexcept>

using namespace Contextual;


namespace Contextual {
	bool _WITH_DESTROYED = false;
	bool _RESOURCE_DESTROYED = false;
	bool _CONTEXT_DESTROYED = false;

	struct IData {
		std::string username;
		std::string password;
		bool logged_in = false;
	};



	class Resource : public IResource<IData> {
	private:
		std::string _password = "xxxxxxxxx";
		IData _data;
		bool hide=true;
		bool reraise = false;
		void enter() override {
			if (hide){
				std::swap(_password, resources->password);
			}
		}

		void exit(std::optional<std::exception> e) override {
			if (hide) {
				std::swap(_password, resources->password);
			}
			resources->logged_in = true;
			if (reraise) {
				throw e;
			}

		}
	public:
		Resource(IData* resources): IResource<IData>(resources){};
		Resource(IData &resources, bool hide=true, bool reraise=false): IResource<IData>(resources),
																  	    hide(hide){

		};
		Resource(IData &&resources): IResource<IData>(&resources){};
		Resource(std::string username, std::string password) : IResource<IData>(_data),
															   _data(IData{username, password})
		 													   {};
	


	};



	class _With : public With {
	public:
		~_With(){
			_WITH_DESTROYED = true;
			
		}
	};

	class _Resource : public Resource {
	public:
		_Resource(IData &resources): Resource(resources) {};
		~_Resource(){
			_RESOURCE_DESTROYED = true;
			
		}
	};

	class _Context : public Context {
	public:
		_Context(std::function<void(IData*)> func) : Context(func) {};
		
		~_Context(){
			_CONTEXT_DESTROYED = true;
		}
	};
};


TEST_CASE("Test core functionality", "[core-functionaliy]"){

	IData data{"admin", "password123"};

	SECTION("Test resource acquisition"){
		With {
			Resource(data, false) + Context{
				[&](auto resource){
					REQUIRE(resource->username == "admin");
					REQUIRE(resource->password == "password123");
				}
			}
		};

	}

	SECTION("Test resource release normal"){
		With {
			Resource(data) + Context{
				[&](auto resource){
					REQUIRE(resource->username == "admin");
					REQUIRE(resource->password == "xxxxxxxxx");
				}
			}
		};

		REQUIRE(data.username == "admin");
		REQUIRE(data.password == "password123");
	}

	SECTION("Test resource release with exceptions"){
		With {
			Resource(data) + Context{
				[&](auto resource){
					REQUIRE(resource->username == "admin");
					REQUIRE(resource->password == "xxxxxxxxx");
					throw std::runtime_error("");
				}
			}
		};

		REQUIRE(data.username == "admin");
		REQUIRE(data.password == "password123");	
	}

	SECTION("Test side-effects"){
		REQUIRE(data.logged_in == false);
		With {
			Resource(data) + Context{
				[&](auto resource){
					resource->logged_in = true;
				}
			}
		};

		REQUIRE(data.logged_in == true);
	}

	SECTION("Test that code block can access variables in outer scope"){
		int GLOBAL_VAR = 0;
		
		With {
			Resource(data) + Context{
				[&](auto resource){
					++GLOBAL_VAR;
				}
			}
		};		
		REQUIRE(GLOBAL_VAR == 1);
	}

}

TEST_CASE("Test cleanup", "[cleanup]"){
	IData data{"admin", "password123"};

	_WITH_DESTROYED = false;
	_RESOURCE_DESTROYED = false;
	_CONTEXT_DESTROYED = false;

	SECTION("Test destructors are called in anticipated use case"){
		REQUIRE(_WITH_DESTROYED == false);
		REQUIRE(_RESOURCE_DESTROYED == false);
		REQUIRE(_CONTEXT_DESTROYED == false);

		_With {
			_Resource(data) + _Context{
				[&](auto resource){
				}
			}
		};		
		REQUIRE(_WITH_DESTROYED == true);
		REQUIRE(_RESOURCE_DESTROYED == true);
		REQUIRE(_CONTEXT_DESTROYED == true);
		
	}

	SECTION("Test destructors are called even with trying to store with object"){

		// Note that naming the context manager means it will not be destoyed
		REQUIRE(_RESOURCE_DESTROYED == false);
		REQUIRE(_CONTEXT_DESTROYED == false);

		_With Temp{
			_Resource(data) + _Context{
				[&](auto resource){
				}
			}
		};		
		
		REQUIRE(_RESOURCE_DESTROYED == true);
		REQUIRE(_CONTEXT_DESTROYED == true);
		
	}

	SECTION("Test destructors are called even with trying to assign object"){
		// The _With object is preserved. Ideally this would not happed
		// but copy elision prevents the deletion of assignment in this case.

		REQUIRE(_RESOURCE_DESTROYED == false);
		REQUIRE(_CONTEXT_DESTROYED == false);

		_With Temp = _With {
			_Resource(data) + _Context{
				[&](auto resource){
				}
			}
		};		
		
		REQUIRE(_RESOURCE_DESTROYED == true);
		REQUIRE(_CONTEXT_DESTROYED == true);
		
	}

	SECTION("Test destructors are called even with exceptions"){

		REQUIRE(_WITH_DESTROYED == false);
		REQUIRE(_RESOURCE_DESTROYED == false);
		REQUIRE(_CONTEXT_DESTROYED == false);

		_With {
			_Resource(data) + _Context{
				[&](auto resource){
					throw std::runtime_error("");
				}
			}
		};		
		REQUIRE(_WITH_DESTROYED == true);
		REQUIRE(_RESOURCE_DESTROYED == true);
		REQUIRE(_CONTEXT_DESTROYED == true);
		
	}

	SECTION("Test destructors are called in withless block") {
		// This clearly destroys the With object as the return
		// of the + expression is not stored
		REQUIRE(_RESOURCE_DESTROYED == false);
		REQUIRE(_CONTEXT_DESTROYED == false);
		_Resource(data) + _Context{
				[&](auto resource){
					
				}
			};

		
		REQUIRE(_RESOURCE_DESTROYED == true);
		REQUIRE(_CONTEXT_DESTROYED == true);
	}
}

TEST_CASE("Test exception logic", "[exceptions]"){
	IData data{"admin", "password123"};

	SECTION("Test exception suppression"){
		With {
			Resource(data) + Context{
				[&](auto resource){
					throw std::runtime_error("");
					REQUIRE(false);
				}
			}
		};	
		REQUIRE(true);
	}

	SECTION("Test exception propogation"){
		try{
			With {
				Resource(data, true, true) + _Context{
					[&](auto resource){
						throw std::runtime_error("TEST");
						REQUIRE(false);
					}
				}
			};	
		} catch (std::runtime_error& e) {
			REQUIRE(e.what() == std::string("TEST"));
		}
	}
}

TEST_CASE("Test syntax features", "[syntax]"){
	IData data{"admin", "password123"};

	SECTION("Test nameless 'Withless' block"){
		REQUIRE(data.username == "admin");
		REQUIRE(data.password == "password123");
		Resource(data) + Context{
				[&](auto resource){
					REQUIRE(resource->username == "admin");
					REQUIRE(resource->password == "xxxxxxxxx");
				}
			};

		REQUIRE(data.username == "admin");
		REQUIRE(data.password == "password123");
	
	}
	
	SECTION("Test named 'With' block"){
		REQUIRE(data.username == "admin");
		REQUIRE(data.password == "password123");
		With PasswordHidden{
			Resource(data) + Context{
				[&](auto resource){
					REQUIRE(resource->username == "admin");
					REQUIRE(resource->password == "xxxxxxxxx");
				}
			}
		};

		REQUIRE(data.username == "admin");
		REQUIRE(data.password == "password123");
	}

	SECTION("Test IData emplace"){
		
		With PasswordHidden{
			Resource(IData{"admin", "password123"}) + Context{
				[&](auto resource){
					REQUIRE(resource->username == "admin");
					REQUIRE(resource->password == "xxxxxxxxx");
				}
			}
		};
	}

	SECTION("Test internal IData construction"){
		
		With {
			Resource("admin", "password123") + Context{
				[&](auto resource){
					REQUIRE(resource->username == "admin");
					REQUIRE(resource->password == "xxxxxxxxx");
				}
			}
		};
	}

	SECTION("Test pointer initialization"){
		REQUIRE(data.username == "admin");
		REQUIRE(data.password == "password123");
		With {
			Resource(&data) + Context{
				[&](auto resource){
					REQUIRE(resource->username == "admin");
					REQUIRE(resource->password == "xxxxxxxxx");
				}
			}
		};

		REQUIRE(data.username == "admin");
		REQUIRE(data.password == "password123");
	}


}