# Arduino to MI Band 2
This works with early adafruit firmware, something like 0.8.6.  See my other project feather_cgms/feather_cgms3.ino for code that works with 0.11.

Initially run with authenticated =0.  This will pair the MI with your Board.  The MI will require an acknowledgment as part of the paring. After that, change authenticated to 1.  

Main point of this is to get custom text messages to display on a MI 2.
Does an sms alert as is.  Uncomment alertTest for a sms alert that requires acknowledgement.
Also will do basic vibratory alerts with no text.

This also works on the Amazfit Cor.
