# 07 — Haze is lit by the directional lights, not by the sky

## Seen
After bug 06 was planned as a haze start (0414), the sponsor: "I am not sure I got through. I want haze to not
get light from sky. Haze should be lit from directional light(s)."

Today the haze colour is the sky's gradient at the horizon times the sky's strength (0411 point 4, 0412). So
the sky's colours and strength reach every hazed surface, and the sky acts as a light through the haze.

## Expected
The haze takes its colour and brightness from the directional lights, not from the sky. The sky's colours and
strength do not change how the haze looks. With no directional light, the haze adds no light.

This reverses 0411 point 4 ("Haze is the sky's … Lights' glows do not tint the haze"), which the sponsor had
agreed to. It is the tech-lead's decision how the lights light the haze (colour times strength, fill, more than
one light, the direction of view against the light), and whether 0414's haze start stays.

## How to reproduce
1. Open lumberhill_project in the editor, with the sky on and haze above 0.
2. Change the sky's colours or strength: the hazed surfaces change with them.
3. Change the directional light's colour or strength: the haze does not follow it.
