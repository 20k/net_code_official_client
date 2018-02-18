function(context, args)
{		
	//ns_call("test.hello");

	#hs.i20k.test_freeze;
	
	//return ms_call("i20k.test_freeze")();
	
	JSON = "hi";
	
	return JSON;
	
	//return #fs.i20k.test_freeze();
	
	//return fs_call("i20k.test_freeze")();
			
	//return #fs.i20k.test_freeze();
	
	return JSON.stringify("hello");
	
	return context.caller;
	
	return "This is my script"
}