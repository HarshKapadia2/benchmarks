# CPU Cache Benchark Plots

## Usage Instructions

- Install Gnuplot

    ```bash
    $ sudo apt-get update
    $ sudo apt-get install gnuplot
    ```

- Navigate to the plot directory

- Plot graphs

    ```bash
    $ gnuplot --persist --slow cache-size.p
    # First render might take time due to missing font cache. Subsequent renders
    # should be faster.
    # Ignore 'no terminal output' warnings
    ```
