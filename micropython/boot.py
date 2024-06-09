# hack from https://github.com/fizista/micropython-umqtt.simple2/tree/master#problems-and-solutions
# to make sure that umqtt2 is loaded instead of the buildin umqtt module
import sys
sys.path.reverse()