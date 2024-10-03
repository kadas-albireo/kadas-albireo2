#ifndef KADASMAPITEMINTERFACE_H
#define KADASMAPITEMINTERFACE_H



#include "kadas/gui/kadas_gui.h"

class KadasMapItem;

/**
 * An interface for map items creation
 */
class KADAS_GUI_EXPORT KadasMapItemInterface
{
  public:
    KadasMapItemInterface() {}

    virtual ~KadasMapItemInterface() {}

    virtual KadasMapItem* createItem() const = 0;
};

#endif // KADASMAPITEMINTERFACE_H
