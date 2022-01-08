# Java Injector
Dll-library for packed *.class injection in JVM

# Usage:
1. Inject *.dll to related Java process
2. Choose *.jar/*.zip to inject
3. Enjoy

# Injectable file need contain archive comment with folowing content:
1. First line - classPath (example: com.example.Example)
2. Second line - classLoader (If not specified, you should manualy choose loader during injection)

