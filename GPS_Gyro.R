library(ggplot2)
library(ggmap)
library(dplyr)

d = read.csv("GPSLOG0.csv", header = TRUE)

gps.map <- get_map(location = c(long = mean(d$longitude),
                                lat = mean(d$latitude)),
                      zoom = 18,
                      maptype = "satellite",
                      scale = 2)
coordinates = d %>% select(longitude, latitude)
ggmap(gps.map) + geom_point(data= coordinates,
                            aes(x = longitude, y = latitude, color= "red"))
# +
#   geom_point(data=c(as.data.frame(d$longitude), as.data.frame(d$latitude)),
#              aes(x = lon, y = lat, fill = "red", alpha = 0.8),
#              size = 5, shape = 21) +
#   guides(fill=FALSE, alpha=FALSE, size=FALSE)

