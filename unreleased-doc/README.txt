This folder contains the unreleased documentation.

The latex documentation is deprecated in favor of the drupal site.
A drupal documentation "book" contains a copy of the html version of the
formerly latex documentation.  Currently distribute the documentation by
viewing the "print friendly" version of the online tutorial, then use
firefox to save "web page, complete" a file called "tutorial.html".
By using "web page, complete" a folder called "tutorial_files" will also
be created- containing local versions of all images needed for the
tutorial.  A user opening tutorial.html will view the page properly
regardless of browser- so long as the tutorial_files folder is placed in
the same directory.

The unreleased-doc directory has been reduced to contain the dia-based
originals of the diagrams used in the tutorial.  Remember to edit them,
export them as png files, then place them on the drupal website.

Pay attention that the filenames don't change, as that will complicate
things when you commit into mercurial and create tarball distributions.
