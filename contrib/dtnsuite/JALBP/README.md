# JAL Library
The Java Abstraction Layer (JAL) library is the Java wrapping of Abstraction Layer Bundle Protocol (ALBP). JAL library uses JNI methods to use the ALBP library written in C. It allows users to use Bundle Protocol sockets on Java programs without caring about DTN implementation running on the machine. According to ALBP library, the supported DTN implementations are DTN2, ION and IBR-DTN.
## Requirements
To install JAL library you need:
-   Java JDK 1.8 or superior.
-   Ant.
-   ALBP library ([git](https://gitlab.com/dtnsuite/al_bp)).
-   Linux based system (because of compatibility of ALBP library).
-   At least one of the DTN implementations supported by ALBP library (ION, DTN2 and IBR-DTN).

## Download
To download the compiled library visit [this](https://dtnsuite.gitlab.io/JALBP/).
See the [section](#installationALBP) to set up the ALBP library.
After that you can import the jar file in your project.
## Documentation/Javadoc
The documentation is located at the following link: [javadoc](https://dtnsuite.gitlab.io/JALBP/javadoc/).
## Installation
The installation of JAL needs 3 steps:
### <a name="installationALBP"></a>1. Download and installation ALBP library (if not already done)
The first step is to download the ALBP library.
```bash
$ git clone https://gitlab.com/dtnsuite/al_bp
$ cd al_bp
```
Then compile the downloaded library.
- To compile only for DTN2 implementation
    ```bash
    $ make DTN2_DIR=<dtn2_dir>
    ```
- To compile only for ION implementation
    ```bash
    $ make ION_DIR=<ion_dir>
    ```
- To compile only for IBR-DTN implementation
    ```bash
    $ make IBRDTN_DIR=<ibrdtn_dir>
    ```
- To compile for DTN2 and ION implementations
    ```bash
    $ make DTN2_DIR=<dtn2_dir> ION_DIR=<ion_dir>
    ```
- To compile for DTN2 and IBR-DTN implementations
    ```bash
    $ make DTN2_DIR=<dtn2_dir> IBRDTN_DIR=<ibrdtn_dir>
    ```
- To compile for ION and IBR-DTN implementations
    ```bash
    $ make ION_DIR=<ion_dir> IBRDTN_DIR=<ibrdtn_dir>
    ```
- To compile for ION, IBR-DTN and DTN2 implementations
    ```bash
    $ make DTN2_DIR=<dtn2_dir> ION_DIR=<ion_dir> IBRDTN_DIR=<ibrdtn_dir>
    ```
Then install the ALBP library
```bash
$ sudo make install
```
### 2. Download and install JAL library
Download and install the JAL library
```bash
$ git clone https://gitlab.com/dtnsuite/JALBP
$ cd JALBP
# The following make command should be launched with parameters in the same way as in the ALBP library.
$ make
$ sudo make install
$ ant
```
The jar file will be created in `./dist/lib/`.
### 3. Import and use the library
Once created the jar file you can import it into your project and use it. After including the jar file you can specify to your IDE the internal folder “doc” which contains the javadoc documentation.
## How to use the JAL library
1. The JALEngine can use either the IPN or DTN EID scheme. The choice is carried out by default on the basis of the subjected DTN implementation (DTN for DTN2 and IBR-DTN, IPN for ION). However the default may be overridden by forcing the wanted scheme. Pay attention that in case you want ot force the IPN scheme you must set also the IPNSchemeForDTN2 because DTN2 working with IPN scheme does not know which is its local IPN number.
2.  Initialize the JALEngine by calling the init method. This operation is optional because when registering a BPSocket the JALEngine is automatically initialized, allowing users not to worry about that.
3.  Register a BPSocket. In this step you have to specify the demux token which is the equivalent of a TCP/UDP port. You can specify demux token only for DTN scheme, only for IPN scheme or both. You can register as many sockets as you want.
4.  Send a bundle.
5.  Receive a bundle. Let you to receive a bundle addressed to the endpoint ID used on registration phase (step 3).
6.  Unregister a BPSocket. This is the dual of the step 3. It unbinds a BPSockets. You have to unregister every BPSocket before closing the program.
7.  Destroy the JALEngine. This operation must be called when you do not want to use BPSockets anymore and it serves to deallocate the resources properly. It is automatically called when the library for JNI calls is unloaded from the JVM; this time usually coincides with the ending of the program therefore is required that your application destroys the JALEngine when no more BPSockets are created.
[![](https://lh4.googleusercontent.com/C9zANkZoLKXuH13ZqUofG6Nwnt50MLqzu_StwhhKlqriMMytCDkFEjCsqX3I3qxS5aj7de3qhhgfyTz4m0SSnKa6sf2WwUoOZjq5uRD6C9uVVgwr2SmYEsjX84VPoj-i7Ok3FE6O)
Here a flow diagram of the steps that a user have to follow to use the JAL library.
## Usage examples
Some simple examples follow to send and receive bundles and then a simple complete program that sends and receives bundles.
### Send bundle
The following code shows how to send a bundle to ipn:1.4 with replyTo set to dtn:machine2/replyTo and custody request. It registers to the DTN socket with demux token as 8 in case of IPN scheme or “sending” in case of DTN scheme.
```java
BPSocket socket = null;
try {
	socket = BPSocket.register("sending", 8);
} catch (JALRegisterException e) {
	System.err.println("Error on registering.");
	System.exit(1);
}
byte[] data = "the data you want to send".getBytes();
Bundle bundle = new Bundle(BundleEID.of("ipn:1.4")); // Creates a new bundle with destination ipn:1.4
bundle.setPayload(BundlePayload.of(data)); // Setting the payload
bundle.setReplyTo(BundleEID.of("dtn:machine2/replyTo")); // Sets the replyTo to the uri dtn:machine2/replyTo
bundle.setExpiration(120); // Changes the default expiration setting to 120 seconds
bundle.addDeliveryOption(BundleDeliveryOption.Custody); // Add request for custody

try {
	socket.send(bundle);
} catch (NullPointerException | IllegalArgumentException | IllegalStateException | JALNullPointerException | JALNotRegisteredException | JALSendException e) {
	System.err.println("Error on sending bundle.");
	System.exit(1);
}
		
try {
	socket.unregister(); // Closes the socket
} catch (JALNotRegisteredException | JALUnregisterException e) {
	System.err.println("Error on unregistering socket.");
	System.exit(1);
} 

try {
	JALEngine.getInstance().destroy(); // Destroys the engine (after this you can't use anymore BP sockets)
} catch (JALLibraryNotFoundException | JALInitException e) {
	System.err.println("Error on destroying JALEngine.");
	System.exit(1);
}
```
### Receive bundle
To receive a bundle from a DTN socket you need to call the receive function as in the following example. It registers to the DTN socket with demux token as 9 in case of IPN scheme or “receiving” in case of DTN scheme.
```java

BPSocket socket = null;
try {
	socket = BPSocket.register("receiving", 9);
} catch (JALRegisterException e) {
	System.err.println("Error on registering.");
	System.exit(1);
}

try {
	Bundle bundle = socket.receive();
} catch (JALReceptionInterruptedException e) {
	System.err.println("Reception interrupted.");
	System.exit(1);
}
catch (JALTimeoutException e) {
	System.err.println("Timed out.");
	System.exit(1);
}
catch (JALNotRegisteredException | JALReceiveException e) {
	System.err.println("Error on receiving.");
	System.exit(1);
}

try {
	socket.unregister(); // Closes the socket
} catch (JALNotRegisteredException | JALUnregisterException e) {
	System.err.println("Error on unregistering socket.");
	System.exit(1);
} 

try {
	JALEngine.getInstance().destroy(); // Destroys the engine (after this you can't use anymore BP sockets)
} catch (JALLibraryNotFoundException | JALInitException e) {
	System.err.println("Error on destroying JALEngine.");
	System.exit(1);
}
```
### A simple complete program
Now that we know how to send and receive a bundle we can write a simple complete program that will receive a bundle sending the same data received from the source. It registers to the DTN socket with demux token as 10 in case of IPN scheme or “echo” in case of DTN scheme.
```java
BPSocket socket = null;
try {
	socket = BPSocket.register("echo", 10);
} catch (JALRegisterException e) {
	System.err.println("Error on registering.");
	System.exit(1);
}

Bundle bundle = null;
try {
	bundle = socket.receive();
} catch (JALReceptionInterruptedException e) {
	System.err.println("Reception interrupted.");
	System.exit(1);
}
catch (JALTimeoutException e) {
	System.err.println("Timed out.");
	System.exit(1);
}
catch (JALNotRegisteredException | JALReceiveException e) {
	System.err.println("Error on receiving.");
	System.exit(1);
}

bundle.setDestination(bundle.getSource()); // We want to forward the bundle to the source
bundle.setExpiration(120); // Sets the expiration to 120 seconds

try {
	socket.send(bundle);
} catch (NullPointerException | IllegalArgumentException | IllegalStateException | JALNullPointerException
		| JALNotRegisteredException | JALSendException e) {
	System.err.println("Error on sending bundle.");
	System.exit(1);
}

try {
	socket.unregister(); // Closes the socket
} catch (JALNotRegisteredException | JALUnregisterException e) {
	System.err.println("Error on unregistering socket.");
	System.exit(1);
} 

try {
	JALEngine.getInstance().destroy(); // Destroys the engine (after this you can't use anymore BP sockets)
} catch (JALLibraryNotFoundException | JALInitException e) {
	System.err.println("Error on destroying JALEngine.");
	System.exit(1);
}
```
## Uninstall

To uninstall the JAL library you need to run the commands below.
```bash
$ cd <JAL_folder>
$ make clean
$ sudo make uninstall
$ ant clean
```
After doing that, you can stop using the .jar file from your projects.
