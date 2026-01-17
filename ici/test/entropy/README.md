
### README: Randomness Quality Testing Suite

---

#### OVERVIEW
This project provides a solution for evaluating the quality of random byte streams using statistical randomness tests and FIPS compliance checks. The suite includes a script (`dotest`) to automate testing and a utility (`entropy_test`) to generate random bytes.

#### COMPONENTS
1. **Random Byte Generator (`entropy_test`)**
   - Generates random data using a custom entropy source.
   - Produces raw binary output for analysis.
   - Designed to integrate seamlessly with `dotest`.

2. **Testing Script (`dotest`)**
   - Automates the process of generating random data and testing its quality.
   - Supports multiple randomness test utilities:
     - `dieharder` (if installed): A comprehensive suite of statistical tests.
     - `rngtest` (if installed): FIPS 140-2 compliance testing.
     - `ent` (if installed): Basic entropy analysis.

3. **Statistical Testing Utilities**
   - **Dieharder**:
     - Executes selected statistical tests: Birthday Spacings, Overlapping Permutations, and Rank of 32x32 Binary Matrices.
     - Displays results with color-coded assessments: PASSED (green), WEAK (orange), FAILED (red).
   - **RNGTEST**:
     - Performs FIPS 140-2 tests, calculating a quality ratio for randomness.
   - **ENT**:
     - Provides entropy-related statistics such as Shannon entropy and arithmetic mean.

#### USAGE
1. **Setup**
   - Compile the entropy_test generator:
     ```bash
     gcc -o entropy_test entropy_test.c -lici
     ```
   - Ensure `dotest` is executable:
     ```bash
     chmod +x dotest
     ```

2. **Execution**
   - Run the `dotest` script:
     ```bash
     ./dotest
     ```
   - The script will:
     - Verify the presence of required utilities and the `entropy_test` binary.
     - Generate a 10 MB random byte stream.
     - Run the selected randomness tests.
     - Summarize the results with clear, color-coded output.

3. **Direct Use of `entropy_test`**
   - While `entropy_test` is primarily intended for use with `dotest`, it can also be executed directly:
     ```bash
     ./entropy_test > random_data.bin
     ```
   - Generates approximately 10 MB of random data (default settings) for manual analysis.

#### OUTPUT
- **Dotest Summary**:
  - Full outputs of test utilities (`rngtest`, `dieharder`, `ent`) are displayed.
  - Highlights key metrics, such as successes, failures, p-values, and quality ratios.
- **RNGTEST Quality Ratio**:
  - Calculated as `Successes / (Failures + 1)`.
  - A ratio below the configurable threshold triggers a warning.

#### CUSTOMIZATION
- Adjust randomness quality thresholds or modify the set of dieharder tests by editing `dotest`.
- Update the buffer size or number of iterations in `entropy_test.c` to change the default 10 MB output size.

#### LIMITATIONS
- The suite requires at least one of the following utilities to be installed:
  - `dieharder`
  - `rngtest`
  - `ent`
- If none are found, `dotest` will prompt installation and exit.

#### FURTHER DETAILS
- See `dotest` for a detailed explanation of the testing script's logic and flow.
- Refer to `entropy_test.c` for information on how random data is generated.

---

**AUTHOR**: Sky DeBaun
This suite was designed to provide a convenient means of randomness evaluation.
