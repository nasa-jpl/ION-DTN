package gov.nasa.jpl.iondtn.backend;

import android.util.Log;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.io.PrintWriter;

/**
 * Updates the DTN IP Neighbor Discovery (IPND) protocol configuration file
 * based on the general settings of the IonDTN application.
 *
 * @author Robert Wiewel
 */
public class IpndFileUpdater {
    private static final String TAG = "IpndUpdater";

    /**
     * Triggers the update of the IPND configuration file based on the
     * provided parameters
     * @param folderPath The path where the configuration file is located
     * @param status The activation/deactivation status of IPND as selected
     *               by the user
     * @param propEID The EID that is used in the beacons for propagating the own
     *            node.
     * @param listenAdr The address of the form ipv4-adr:port that is used for
     *            propagating the own node
     * @return true on success, false on error
     */
    public static boolean updateConfigFile(String folderPath,
                                           Boolean status,
                                           String propEID,
                                           String listenAdr,
                                           String listeningPort,
                                           String intervalUnicast,
                                           String intervalMulticast,
                                           String intervalBroadcast,
                                           String ttlMulticast,
                                           String destIP,
                                           Boolean tcpclPropagation,
                                           String tcpclPort) {
        // Create a handle for the ipnd config file (that should be residing
        // in the general startup folder (a custom startup script for the
        // ipnd config file is not possible)
        File configFile = new File(folderPath, "node.ipndrc");

        // Check if the file exists
        if (configFile.exists()) {
            // Delete the file if it exists
            if (!configFile.delete()) {
                Log.e(TAG, "updateConfigFile: Failed to delete config file");
            }
        }

        // Recreate the file
        try {
            if (!configFile.createNewFile()) {
                Log.e(TAG, "updateConfigFile: ipnd config file didn't " +
                        "exist and creation failed!");
                return false;
            }
        }
        catch (IOException e) {
            Log.e(TAG, "updateConfigFile: ipnd config file didn't exist " +
                    "and creation failed!");
            return false;
        }

        // Create the new file content
        String content;

        if (status) {
            content = "1 \n" +
                    "m eid " + propEID + " \n" +
                    "m port " + listeningPort + " \n" +
                    "m announce " +
                    "period 1 \n" +
                    "m announce eid 1 \n" +
                    "m interval unicast " + intervalUnicast + " \n" +
                    "m interval multicast " + intervalMulticast + " \n" +
                    "m interval broadcast " + intervalBroadcast + " \n" +
                    "m multicast ttl " + ttlMulticast + " \n" +
                    "a listen " + listenAdr + " \n" +
                    "a destination " + destIP + " \n";

            if (tcpclPropagation) {
                content +="a svcadv CLA-TCP-v4 IP:192.168.8.245 Port:" +
                                tcpclPort +" \ns \n";
            }
            else {
                content += "s \n";
            }
        }
        else {
            content = "";
        }

        try {
            // Reset the file (i.e. write "" to the file)
            PrintWriter writer = new PrintWriter(configFile);
            writer.print("");
            writer.close();

            // Write the new file content
            OutputStream ionConfigOut = new FileOutputStream(configFile,
                    true);
            ionConfigOut.write(content.getBytes());
            ionConfigOut.close();
        }
        catch (FileNotFoundException e) {
            Log.e(TAG, "updateConfigFile: Could not open file! Aborting!");
            return false;
        }
        catch (IOException e) {
            Log.e(TAG, "updateConfigFile: IO Exeption while writing to file! " +
                    "Aborting!");
            return false;
        }

        return false;
    }
}
