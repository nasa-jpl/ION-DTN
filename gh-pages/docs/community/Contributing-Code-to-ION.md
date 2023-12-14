# Contributing Code to ION

## Expectations

If you plan to contribute to the ION project, please keep these in mind:

- Submitted code should adhere to the ION coding style found in the current code.
- Provide documentation describing the contributed code’s features, its inputs and outputs, dependencies, behavior (provide a high-level state machine or flowchart if possible), and API description. Please provide a draft of a man page.
- Provide a canned test (ION configuration and script) that can be executed on a single host to verify the proper functioning of the feature. Ideally it should include both nominal and off-nominal operations.
- The NASA team will review these contributions and determine to either

  1. Incorporate the code into the baseline, or
  2. Make the code available in the `/contrib` folder for continued experimental use,
  3. Not incorporate it at all
- All baselined features will be supported with at least bug-fixes until removed
- All /contrib folder features are provided ”as is,” and no commitment is made regarding bug-fixes.
- The contributor is expected to help with regression testing
- Due to resource constraints, we cannot make any commitment as to response time. We will do our best to review them on a best effort basis.

## If you want to contribute...

1. Fork this repository
2. Starting with the "current" branch, create a named feature or bugfix branch and develop/test your code in this branch
3. Generate a pull request
