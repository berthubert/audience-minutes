#!/usr/bin/awk -f
#  1      2 3   4                     5      6    7                                          8       9   10    11
# 0.0.0.0 - - [26/Jun/2021:00:01:08 +0200] "GET /articles/report.json?scrollPerc=4&count=5 HTTP/1.1" 200 0 "https://berthub.eu/articles/posts/reverse-engineering-source-code-of-the-biontech-pfizer-vaccine/" 

BEGIN { printf("url,perc,count\n"); }
{
    split($11, parts, "?");
    url=parts[1];

    #   0                      1       2 3     4
    # /articles/report.json?scrollPerc=4&count=5
    if($7 ~ /report.json/) {
        split($7, parts, "[?=&]");
    
        printf("%s,%s,%s\n", url, parts[3], parts[5]);
    }
}
