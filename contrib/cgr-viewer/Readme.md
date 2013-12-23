# CGR Viewer

CGR Viewer takes a JSON file output by `cgrfetch` and builds a visualization of
it in webpage form.

# How to Use

First, run `cgrfetch` to obtain a JSON file of a CGR simulation. This looks
something like:

    ionstart -I CONFIG.rc
    cgrfetch -o cgr.json DEST-NODE

See `cgrfetch --help` and `man cgrfetch` for more information on the tool.

Then, load up `cgr-viewer.html` in a web browser and click the Choose JSON
button. Browse to a cgrfetch JSON file and select it. The page will then be
populated with routes from that simulation.

At the very top of the page are three tab buttons -- Parameters, Legend, and
Choose JSON. Click on the Parameters tab to view the parameters of the
simulation -- the local and destination nodes, requested dispatch time,
expiration time, and bundle size and class. Click on the Legend tab to see
descriptions of the symbols used in the route graphs. Click again on the active
tab or on the page background to close the tab menu.

Below the tab menu, each route is represented by a box. At the top of each box
is a classification as determined by CGR -- Selected, Considered, Not
Considered, or Ignored. Directly beneath the classification is the reason CGR
made its decision.

Below the classification and reason are the calculated parameters of the route
-- dispatch time, delivery time, and the max payload/payload class. Hover over
the max payload to see the precise number of bytes.

Finally at the base of each box is a graph of the route. Click anywhere on the
box to view a full-size version of the graph. To return to the normal view,
click anywhere outside the box or hit the escape key.

Use the Choose JSON button to repopulate the page with a different JSON file.

# Requirements

CGR Viewer has been tested with Firefox 17-27 and Chrome 28-30.

# Future Work

 - Test in more browsers
 - Add error handling for invalid JSON, etc.
 - Add specialized print stylesheet for saving to pdf/printing
