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

At the end of this page I've included some discussion how to interpret the
data.

Privacy
-------
These scripts use no cookies and no local storage. There are no identifiers.
You can run this without having to add a cookie or GDPR banner etc. However,
I personaly prefer to sample as little as possible. Many sites will track
your every click, and even note if you switch to another tab. I find that
somewhat upsetting.

You can tailor your level of intrusiveness with the `reportingProbability`
setting. The busier your site is the lower you can set this at and still
have decent statistics.

JavaScript
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

How to interpret the results
----------------------------
For starters, you'll notice that even quite high visitor numbers translate
into not that many "audience minutes". This is not due to this script, it is
a common disappointment. So if your calculations show that an article had
100 reading hours, this actually is quite a lot. 

For obvious reasons, professional media operations are not very forthcoming
with these statistics on their readership. One big newspaper article I
worked on garnered 75 confirmed reading hours, for example.

So why are these numbers so low? For starters, you may not be seeing all
readers. Perhaps the script doesn't fire on all devices. I've done some
research, I don't think this is a major factor. But it could be.

On the other hand, there is a ton of automated traffic coming to sites these
days, lots of crawlers, bots, strange scanners etc. Mind you, some of these
will even execute JavaScript! But most won't. This non-human traffic may
have been inflating your numbers previously.

In terms of the graph, if you have a ton of data, interpretation is easy. I
find that you can even see where you put big photos in an article - these
sections do not get a lot of reading minutes.

If you have less data, you need to reduce the number of 'bins' in the
histogram. I find that 10 bins work pretty well for general conclusions, and
that might get you this:

![](/sample-data/histo10.png)

If you get this, you can conclude that 1) most people that visited the URL
actually wanted to read this kind of content and 2) most readers made it to
the end of your article. The profile is mostly flat, with only some drop-off
near the end, and no suspicious peak at the beginning.

Contrast this with:

![](/sample-data/histo10b.png)

This was an article that was extremely popular on HackerNews and a few other
places. But we can see that readership clearly peaked in the first 10%. Lots
of people decided that they had read enough at that point. This is
caused by how the article got promoted, and it is not necessarily the
"fault" of the writer.

If we ignore the "mistargeting", from that point on, almost everyone makes
it to the end of the article. 


Sample data
-----------
If you want to help improve these small tools, but you don't yet have a lot
data to work with, please find attached two weeks of data collected by 
[berthub.eu/articles](https://berthub.eu/articles) in the
[sample-data](sample-data/) directory.

