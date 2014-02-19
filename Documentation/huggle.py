import huggle;

# This file is just an interface to describe the huggle module, it exist just for doxygen


"""@package huggle
This module is an interface to use huggle. 

In case you want to create an extension for huggle in python, you should import it.
When loaded in huggle, you can access all of following functions.
"""

def huggle_version():
    """Returns a version of a huggle in form of string

    Version numbers are separated with dot symbol and build id is appended to the end
    if the version (this works only if huggle was built from within a git repository)
    """
    return "unknown";
