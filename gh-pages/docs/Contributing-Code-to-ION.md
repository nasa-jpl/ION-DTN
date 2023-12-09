# Contributing Code to ION

## Expectations

If you plan to contribute to the ION project, please keep these in mind:

- Submitted code should adhere to the ION coding style found in the current code. We plan to add a more formal coding style guide in the future.
- Provide documentation describing the contributed code’s features, its inputs and outputs, dependencies, behavior (provide a high-level state machine or flowchart if possible), and API description. Please provide a draft of a man page.
- Provide canned tests (ION configuration and script) that can be executed to verify and demonstrate the proper functioning of the features. Ideally it should demonstrate nominal operation and off-nominal scenarios.
- The NASA team will review these contributions and determine to either

  1. incorporate the code into the baseline, or
  2. not incorporate the code into the baseline but make it available in the /contrib folder (if possible) as experimental modules, or
  3. not incorporate it at all.
- All baselined features will be supported with at least bug-fixes until removed
- All /contrib folder features are provided ”as is,” and no commitment is made regarding bug-fixes.
- The contributor is expected to help with regression testing
- Due to resource constraints, we cannot make any commitment as to response time. We will do our best to review them on a best effort basis.

## If you want to contribute...

1. Fork this repository
2. Starting with the "current" branch, create a named feature or bugfix branch and develop/test your code in this branch
3. Generate a pull request with

   - Your feature or bugfix branch as the Source branch
   - "current" as the destination branch
