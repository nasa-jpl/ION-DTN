package gov.nasa.jpl.iondtn.backend;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.IBinder;
import android.support.annotation.Nullable;
import android.util.Log;

import org.apache.commons.io.FileUtils;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;

/**
 * Manages the file management in the internal storage of the
 * application as a service. This includes the copying of configuration files
 * from the resource folder to the default directory and the parsing and
 * copying of optional custom files.
 *
 * The file management is supposed to be handled asynchronously in an
 * AsyncTask that can be started by {@link #runAsyncTask()}.
 *
 * The class also provides a listener interface that can be used to register
 * a function that is called upon completion of the
 * {@link android.os.AsyncTask}.
 *
 * @author Robert Wiewel
 *
 */

public class ConfigFileManager extends Service {
    private static final String TAG = "ConfigFileManager";
    private ConfigFileManagerListener mListener = null;
    private ConfigFileType mType;
    private long nodeNbr;
    private Context mContext;
    private String defaultPath;
    private String customPath;
    private Uri customConfig;

    /**
     * Inherited method that allows binding of the service. In this case
     * disabled.
     * @param intent The binding intent
     * @return always null as this service does not allow binding
     */
    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    /**
     * Enum type representing the type of configuration executed at the first
     * startup of ION
     */
    public enum ConfigFileType {EMPTY, CUSTOM}

    /**
     * Listener interface to allow GUI to receive feedback about finished
     * setup process
     *
     * @author Robert Wiewel
     */
    public interface ConfigFileManagerListener {
        /**
         * Called when configuration process is finished in {@link AsyncTask}.
         * @param status 1 when configuration finished successful, 0 otherwise
         */
        void notifyFinished (boolean status);
    }

    /**
     * Assigns the provided node number and adds this node number to the
     * configuration files during setup
     * @param number_ The local node number (for the IPN scheme)
     */
    public void setNodeNbr(long number_) {
        this.nodeNbr = number_;
    }

    /**
     * Sets the listener object that is used for signaling the GUI when the
     * setup process is finished successfully
     * @param listener_ The listener object that implements the interface
     * {@link ConfigFileManagerListener}
     */
    public void setListener(ConfigFileManagerListener listener_) {
        this.mListener = listener_;
    }

    /**
     * Sets the configuration type that is created during the setup
     * generation process
     * @param type_ The configuration type, either empty or custom
     */
    public void setType (ConfigFileType type_) {
        mType = type_;
    }

    /**
     * Sets the internal configuration to a custom configuration and assigns
     * the internal path variable to the path of the custom config
     * @param config_ The path to the custom configuration file
     */
    public void setCustomConfig(Uri config_) {
        mType = ConfigFileType.CUSTOM;
        customConfig = config_;
    }

    /**
     * Constructor that initializes the internal variables to default values
     * before the set-functions can be used for modifications.
     * @param context_ The application context (necessary for determining
     *                 local paths)
     */
    public ConfigFileManager(Context context_) {
        mType = ConfigFileType.EMPTY;
        nodeNbr = 1;
        mContext = context_;
        customConfig = null;
    }

    /**
     * Executes the configuration files based on the previously chosen
     * configuration options in an {@link AsyncTask}.
     */
    public void runAsyncTask() {
        if (mType == ConfigFileType.CUSTOM) {
            new FileManagerAsyncTask().execute(customConfig);
        }
        else {
            new FileManagerAsyncTask().execute();
        }
    }

    /**
     * {@link AsyncTask} that asynchronously performs the setup files
     * generation. Signals the callee (i.e.) via
     * {@link ConfigFileManagerListener} interface when the setup process is
     * finished.
     */
    private class FileManagerAsyncTask extends AsyncTask<Uri, Void,
            Integer> {
        /**
         * Default constructor to start the generation process.
         */
        FileManagerAsyncTask() {}
        private boolean success = true;

        @Override
        protected Integer doInBackground(Uri... files) {
            success &= createStartFiles();
            if (mType == ConfigFileType.CUSTOM && files.length > 0) {
                success &= createCustomConfigFiles(files[0]);
            }
            return 0;
        }

        @Override
        protected void onPostExecute(Integer integer) {
            if (mListener != null) {
                mListener.notifyFinished(success);
            }
            super.onPostExecute(integer);
        }
    }

    /**
     * Function that creates default startup files that are used by ION from
     * the second startup onwards (or from the beginning, if an "empty"
     * configuration was chosen.
     * @return true the file generation process was successful, false on error
     */
    private boolean createStartFiles() {
        // Create target directory
        File filesDir = mContext.getFilesDir();
        File destDir = new File(filesDir, "StartupConfig");

        // Delete existing config files if present
        try {
            if (filesDir.list() != null) {
                FileUtils.cleanDirectory(filesDir);
            }
        }
        catch (IOException e) {
            Log.e(TAG, "copyStartFiles: Cleaning already existing directory " +
                    "failed!");
            return false;
        }

        if (!destDir.exists() && !destDir.mkdir()) {return false;}

        // Copy all config files from srcDir to destDir
        try {
            File bprcFile = new File(destDir, "node.bprc");
            if (!bprcFile.createNewFile()) {return false;}
            FileUtils.copyToFile(mContext.getAssets().open
                            ("ConfigStartup/node.bprc"), bprcFile);

            File cfdprcFile = new File(destDir, "node.cfdprc");
            if (!cfdprcFile.createNewFile()) {return false;}
            FileUtils.copyToFile(mContext.getAssets().open
                    ("ConfigStartup/node.cfdprc"), cfdprcFile);

            File ionconfigFile = new File(destDir, "node.ionconfig");
            if (!ionconfigFile.createNewFile()) {return false;}
            FileUtils.copyToFile(mContext.getAssets().open
                    ("ConfigStartup/node.ionconfig"), ionconfigFile);

            File ionsecFile = new File(destDir, "node.ionsecrc");
            if (!ionsecFile.createNewFile()) {return false;}
            FileUtils.copyToFile(mContext.getAssets().open
                    ("ConfigStartup/node.ionsecrc"), ionsecFile);

            OutputStream ionConfigOut = new FileOutputStream(ionconfigFile,
                    true);
            String content = "pathName " + mContext.getFilesDir().getAbsolutePath() +
                    "\n";

            ionConfigOut.write(content.getBytes());
            ionConfigOut.close();
        }
        catch (IOException e) {
            Log.e(TAG, "copyStartFiles: Copying files " +
                    "failed!");
            return false;
        }

        // Create .ionrc file with custom node number
        File destIonrc = new File(destDir, "node.ionrc");
        OutputStream outStr;

        String content = "1 "
                + Long.toString(nodeNbr)
                + " node.ionconfig"
                + "\nm usage\ns\n";

        try {
            outStr = new FileOutputStream(destIonrc);
            outStr.write(content.getBytes());
            outStr.close();
        }
        catch (IOException e) {
            Log.e(TAG, "copyStartFiles: Couldn't create ionrc file!");
            return false;
        }

        // Setup the destination directory as default startup path
        defaultPath = destDir.getAbsolutePath();

        return true;
    }

    /**
     * Creates a custom configuration based on a file provided as parameter.
     * This custom configuration is executed once at the initial startup.
     * Afterwards the empty startup configuration files generated in
     * {@link #createStartFiles()} are used.
     * @param fileuri The Uri pointing to the custom configuration file
     * @return true when generation was successful, false on error
     */
    private boolean createCustomConfigFiles(Uri fileuri) {
        boolean success;

        // Create target directory
        File filesDir = mContext.getFilesDir();
        File destDir = new File(filesDir, "CustomConfig");

        if (!destDir.mkdir()) {return false;}

        // Delete existing config files if present
        try {
            if (destDir.list() != null) {
                FileUtils.cleanDirectory(destDir);
            }
        }
        catch (IOException e) {
            Log.e(TAG, "createCustomConfigFiles: Cleaning already existing directory " +
                    "failed!");
            return false;
        }

        // Copy all config files from srcDir to destDir
        try {
            File ionconfigFile = new File(destDir, "node.ionconfig");
            if (!ionconfigFile.createNewFile()) {return false;}
            FileUtils.copyToFile(mContext.getAssets().open
                    ("ConfigStartup/node.ionconfig"), ionconfigFile);

            OutputStream ionConfigOut = new FileOutputStream(ionconfigFile,
                    true);
            String content = "pathName " + mContext.getFilesDir().getAbsolutePath() +
                    "\n";

            ionConfigOut.write(content.getBytes());
            ionConfigOut.close();
        }
        catch (IOException e) {
            Log.e(TAG, "copyCustomFiles: Copying files " +
                    "failed!");
            return false;
        }

        success = parseToFile(destDir, "node.ionrc", fileuri, "## begin ionadmin",
                "## end " +
                "ionadmin");
        success &= parseToFile(destDir, "node.bprc", fileuri, "## begin bpadmin",
                "## end " +
                "bpadmin");
        success &= parseToFile(destDir, "node.ipnrc", fileuri, "## begin ipnadmin",
                "## end " +
                "ipnadmin");
        //TODO define which admin programs are required and which are optional
        parseToFile(destDir, "node.ionsecrc", fileuri, "## begin " +
                        "ionsecadmin", "## end ionsecadmin");
        parseToFile(destDir, "node.ltprc", fileuri, "## begin ltpadmin",
                "## end ltpadmin");
        parseToFile(destDir, "node.cfdprc", fileuri, "## begin cfdpadmin",
                "## end cfdpadmin");
        parseToFile(destDir, "node.dtn2rc", fileuri, "## begin dtn2admin",
                "## end dtn2admin");
        parseToFile(destDir, "node.dtpcrc", fileuri, "## begin dtpcadmin",
                "## end dtpcadmin");
        parseToFile(destDir, "node.acsrc", fileuri, "## begin acsadmin",
                "## end acsadmin");
        parseToFile(destDir, "node.imcrc", fileuri, "## begin imcadmin",
                "## end imcadmin");
        parseToFile(destDir, "node.bssrc", fileuri, "## begin bssadmin",
                "## end bssadmin");

        customPath = destDir.getAbsolutePath();

        return success;
    }

    /**
     * Returns the correct initialization path based on the internal
     * configuration
     * @return absolute initialization path as String
     */
    public String getInitPath() {
        if (this.mType == ConfigFileType.EMPTY) {
            return this.defaultPath;
        }
        else {
            return this.customPath;
        }
    }

    /**
     * Returns the path of the generated statup files
     * @return absolute startup path as String
     */
    public String getStartupPath() {
        return this.defaultPath;
    }

    /**
     * Parses a custom configuration file to extract a part that is relevant
     * for a certain administration application
     * @param destDir File object representing the destination directory
     *                where the file shall be saved after extraction
     * @param fileName The name of the target configuration file
     * @param sourceFile An URI pointing to the rc configuration file
     *                   containing configuration instructions for multiple
     *                   administration applications.
     * @param startDelimiter The start delimiter identifying the relevant
     *                       part for an particular administration application.
     * @param endDelimiter The end delimiter identifying the relevant part of
     *                     an particular administration application.
     * @return true when parsing was successful, false on error (also when
     * relevant part was not found in file)
     */
    private boolean parseToFile(File destDir, String fileName, Uri
            sourceFile, String startDelimiter, String endDelimiter) {
        InputStream fileStream;
        try {
            fileStream = mContext.getContentResolver()
                    .openInputStream(sourceFile);
        }
        catch (FileNotFoundException e) {
            Log.e(TAG, "parseToFile: Source file not found");
            return false;
        }

        if (fileStream == null) {
            Log.e(TAG, "parseToFile: FileStream was null!");
            return false;
        }

        BufferedReader lineReader = new BufferedReader(
                new InputStreamReader(fileStream));

        String line;

        try {
            // Find position of start delimiter in file
            while (true) {
                if ((line = lineReader.readLine()) == null) {
                    Log.e(TAG, "parseToFile: Start delimiter not found in " +
                            "file");
                    lineReader.close();
                    fileStream.close();
                    return false;
                }
                else if (line.contains(startDelimiter)) {
                    break;
                }
            }

            // Create config file (and output stream for it)
            File file = new File(destDir, fileName);
            if (!file.createNewFile()) {
                Log.e(TAG, "parseToFile: Failed creating new file!");
                lineReader.close();
                fileStream.close();
                return false;
            }
            OutputStream outStream = new FileOutputStream(file);

            // Write config lines to file
            while (true) {
                if ((line = lineReader.readLine()) == null) {
                    Log.e(TAG, "parseToFile: File ended before end delimiter " +
                            "was found");
                    if (!file.delete()) {
                        Log.e(TAG, "Deleting temporary file failed!");
                    }
                    lineReader.close();
                    fileStream.close();
                    outStream.close();
                    return false;
                }
                else if (line.contains(endDelimiter)) {
                    break;
                }
                if (fileName.equals("node.ionrc") && line.length() != 0 &&
                        line.charAt(0) == '1') {
                    String content = "1 "
                            + Long.toString(nodeNbr)
                            + " node.ionconfig";
                    outStream.write(content.getBytes());
                    outStream.write("\n".getBytes());

                } else {
                    outStream.write(line.getBytes());
                    outStream.write("\n".getBytes());
                }
            }

            outStream.close();
            lineReader.close();
            fileStream.close();
        }
        catch (IOException e) {
            Log.e(TAG, "parseToFile: IOException while parsing source file!");
            return false;
        }

        return true;
    }

    public long getNodeNumber() {
        return nodeNbr;
    }
}
