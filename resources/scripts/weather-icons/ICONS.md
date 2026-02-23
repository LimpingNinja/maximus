# Weather Icon Mapping

ANS icons sourced from x84 BBS (AccuWeather icon set, originally 1.ans–44.ans).
Panel size: 15 cols × 8 rows (CP437).

## WeatherAPI condition.code → ANS filename

Use `current.is_day` (1=day, 0=night) to pick day vs night variant.

### Day icons

| File | WeatherAPI codes |
|------|-----------------|
| sunny.ans | 1000 |
| mostly-sunny.ans | *(unused — no WA equivalent, fallback for is_day=1 clear)* |
| partly-sunny.ans | 1003 |
| intermittent-clouds.ans | 1003 (alt) |
| hazy-sun.ans | 1030 |
| mostly-cloudy.ans | 1006 |
| cloudy.ans | 1006 (alt) |
| overcast.ans | 1009 |
| fog.ans | 1030, 1135, 1147 |
| showers.ans | 1153, 1183, 1240, 1243 |
| mostly-cloudy-showers.ans | 1186, 1189, 1195 |
| partly-sunny-showers.ans | 1063, 1150, 1180, 1192 |
| thunderstorm.ans | 1273, 1276, 1279, 1282 |
| mostly-cloudy-tstorm.ans | 1087 |
| partly-sunny-tstorm.ans | 1087 (alt) |
| rain.ans | 1189, 1195, 1246 |
| flurries.ans | 1255 |
| mostly-cloudy-flurries.ans | 1066, 1114 |
| partly-sunny-flurries.ans | 1066, 1210, 1216 |
| snow.ans | 1117, 1213, 1219, 1222, 1225, 1258 |
| mostly-cloudy-snow.ans | 1114, 1117 (blizzard) |
| ice.ans | 1237, 1261, 1264 |
| sleet.ans | 1069, 1204, 1207, 1249, 1252 |
| freezing-rain.ans | 1072, 1168, 1171, 1198, 1201 |
| rain-snow.ans | 1069 (mix) |
| hot.ans | 1000 + high temp (optional) |
| cold.ans | 1000 + low temp (optional) |

### Night icons

| File | WeatherAPI codes |
|------|-----------------|
| clear-night.ans | 1000 |
| mostly-clear-night.ans | *(fallback clear night)* |
| partly-cloudy-night.ans | 1003 |
| intermittent-clouds-night.ans | 1003 (alt) |
| hazy-moon.ans | 1030 |
| mostly-cloudy-night.ans | 1006, 1009 |
| partly-cloudy-showers-night.ans | 1063, 1150, 1180, 1240 |
| mostly-cloudy-showers-night.ans | 1153, 1183, 1186, 1189, 1195, 1243, 1246 |
| partly-cloudy-tstorm-night.ans | 1087, 1273 |
| mostly-cloudy-tstorm-night.ans | 1276, 1279, 1282 |
| flurries-night.ans | 1066, 1114, 1210, 1216, 1255 |
| snow-night.ans | 1117, 1213, 1219, 1222, 1225, 1258 |
