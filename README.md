# Inject DLL

Inject code into an open process. 
This enables you to run your code inside the process memory of the target process.
You can do things like `SetWindowsHook` once inside, and hook messages, etc.

For a double-banger effect, inject the Trampoline.dll, which detours DLL calls.

## Notes
* If the process to be injected is 32 bit, then the DLL must also be 32 bit.  The injector has to also be 32 bit.
* Likewise, if the process is 64 bit, then the DLL and injector must also be 64 bit.
* You can also inject shell code instead of a DLL, but that has a size limit, and the shell code must be in machine code. Much easier and convenient to use a DLL.
* It's also possible to have it so the DLL is permanently loaded by the process each time.
Have a look at https://reverseengineering.stackexchange.com/questions/21000/permanently-load-a-dll-to-an-executable for more.
