# -*- coding: utf-8 -*-
"""Custom exception class for Kadas Routing Plugin.
"""


class ValhallaException(Exception):
    pass


class Valhalla400Exception(ValhallaException):
    """Custom exception with detailed message from Valhalla"""

    pass
