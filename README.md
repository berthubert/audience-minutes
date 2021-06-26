# audience-minutes
generate statistics on the number of audience minutes your site is
receiving, and if readers make it to the end of your screeds.

> “If we have data, let’s look at data. If all we have are opinions, let’s go with mine.” - Jim Barksdale.

![There is some sample data linked from the end of this file](/sample-data/graph.png)

What is this?
-------------
If you publish articles, you probably want to know if people are reading
them. You can count raw hits but especially if you aren't drawing a lot of
traffic, many of these hits are actually bots and crawlers. 

In addition, that the page got loaded doesn't tell you if the visitor
actually read your work.

Some time ago I was involved with a Dutch newspaper article, and they could
tell me how many minutes of reading time it had generated. And this made me
somewhat jealous.

With these three scripts, you can instrument your pages with a tiny bit of
javascript that probabilistically samples if a reader was active over the
past minute (mouse/touch movement or scrolling). In the default settings,
10% of every active minute will be reported.  

Or in other words, if you get 10 reports, something like 100 minutes was
spent reading your site. Sorta. 

In addition, these reports measure at what percentage of the content your
reader is positioned. This helps you determine if people are making it to
the end of your page or not.

Privacy
-------
These scripts use no cookies and no local storage. There are no identifiers.
You can run this without having to add a cookie or GDPR banner etc. 

Javascript
----------
Insert or link [audience-minutes.js](audience-minutes.js) from all pages you
want to measure on. There are some settings at the beginning of the file
where you can tweak how intrusive you want the measurements to be. In other
words, do you want to sample 10% of every viewing minute? Or 1%? The busier
your site is the lower you can set this.

In the same place where you put the javascript file, also put an empty file
called `report.json`. This will receive reports of active minutes. If you
don't generate this file, the browser console will show 404s which is ugly.

The easiest way to parse the results is to grab them from an access.log
file.

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

Sample data
-----------
If you want to help improve these small tools, but you don't yet have a lot
data to work with, please find attached two weeks of data collected by 
[berthub.eu/articles](https://berthub.eu/articles) in the
[sample-data](sample-data/) directory.