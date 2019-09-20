/** 
 * Main package for DTN connections based on JALBP library.
 * <p>
 *  It contains the main classes for opening a {@link it.unibo.dtn.JAL.BPSocket} and for sending {@link it.unibo.dtn.JAL.Bundle}s over a DTN connection.</p>
 *  
 *  <p>
 *  To use this library you have to follow some easy steps:
 *  	<ol>
 *  		<li>Get the instance and set the parameters of {@link it.unibo.dtn.JAL.JALEngine} before calling {@link it.unibo.dtn.JAL.JALEngine#init()} function (optional step).</li>
 *  		<li>Register one or more {@link it.unibo.dtn.JAL.BPSocket}s.</li>
 *  		<li>Send/receive {@link it.unibo.dtn.JAL.Bundle}s.</li>
 *  		<li>Close the {@link it.unibo.dtn.JAL.BPSocket}s.</li>
 *  		<li>When you will not use anymore the library you should call {@link it.unibo.dtn.JAL.JALEngine#destroy()}.</li>
 *  	</ol>
 *  </p>
 *  
 * @author Andrea Bisacchi
 * @version 1.0
 */
package it.unibo.dtn.JAL;