# The following has been generated automatically from kadas/gui/kadassearchprovider.h
try:
    KadasSearchProvider.SearchResult.__attribute_docs__ = {'categoryPrecedence': 'Lower number means higher precedence.\n1: coordinate\n2: pins\n10: local features\n11: remote features\n20: municipalities\n21: cantons\n22: districts\n23: places\n24: plz codes\n25: addresses\n30: world locations\n100: unknown'}
    KadasSearchProvider.SearchResult.__annotations__ = {'categoryPrecedence': int}
except (NameError, AttributeError):
    pass
try:
    KadasSearchProvider.__virtual_methods__ = ['cancelSearch']
    KadasSearchProvider.__abstract_methods__ = ['startSearch']
except (NameError, AttributeError):
    pass
