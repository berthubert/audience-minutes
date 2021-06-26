# audience-minutes
generate statistics on the number of audience minutes your site is
receiving, and if readers make it to the end of your screeds.

> “If we have data, let’s look at data. If all we have are opinions, let’s go with mine.” - Jim Barksdale.



Javascript
----------
Insert or link [audience-minutes.js](audience-minutes.js) from all pages you
want to measure on. There are some settings at the beginning of the file
where you can tweak how intrusive you want the measurements to be. In other
words, do you want to sample 10% of every viewing minute? Or 1%? The busier
your site is the lower you can set this.

AWK
---
Yes, AWK! With [this little script](repextract.awk) you can trawl your access.log files and
generate a CSV file that only has URLs, scroll percentages and minute
counts. 

This CSV file has no privacy considerations, there are no IP addresses in
there. Unlike your original access.log. 

Jupyter notebook
----------------
To turn the CSV file into a graphs, use [this Jupyter
script](audience.ipynb), from which you can also extract Python 3 if you
don't want to run Jupyter. It is based on Pandas and Matplotlib.