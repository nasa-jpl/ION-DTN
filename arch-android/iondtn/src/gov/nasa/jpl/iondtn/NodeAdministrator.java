/*
 * Copyright (C) 2012 California Institute of Technology
 */
package gov.nasa.jpl.iondtn;

import android.app.Activity;
import android.widget.TextView;
import android.os.Bundle;

public class NodeAdministrator extends Activity
{
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        /* Create a TextView and set its content.
         * the text is retrieved by calling a native
         * function.
         */
        TextView  tv = new TextView(this);
        tv.setText(startNode());
        setContentView(tv);
    }

    /* Native methods that are implemented by the
     * 'iondtn' native library, which is packaged
     * with this application.
     */
    public native String	init();

    static {
        System.loadLibrary("iondtn");
    }
}
