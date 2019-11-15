# farbot
FAbian's Realtime Box o' Tricks

This is a collection of design patterns which can be used in realtime code. This library was introduced to the public at meeting C++ on 16th of November 2019.

It contains the following classes: NonRealtimeMutatable, RealtimeMutatable and fifo

It also contains a set of useful traits to check if objects are realtime movable, constructable in RealtimeTraits.hpp:

farbot::is_realtime_copy_assignable, farbot::is_realtime_copy_constructable
farbot::is_realtime_move_assignable, farbot::is_realtime_move_constructable