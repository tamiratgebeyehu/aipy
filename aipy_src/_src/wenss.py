"""
The WENSS Catalog.

Data files are in tab-separated format from Vizier.
To download in the correct format, open a catalog online in Vizier,
select'Tab-Separated-Values' as the Output layout in the drop-down box, set
the maximum entries to 'unlimited', and click 'Sexagesimal' under the box
for 'Target Name or Position'.  Submit the query, and copy the output to a
txt file.  Copy this file to "wenss.txt" in the _src directory of your AIPY
installation.
"""

from __future__ import print_function, division, absolute_import

try:
    import aipy as a
except ImportError:
    import aipy_src as a
import numpy as np, os

class WenssCatalog(a.fit.SrcCatalog):
    def fromfile(self,filename):
        f = open(filename)
        addsrcs = []
        for L in [L for L in f.readlines() if not L.startswith('#')]:
            text = L.split('\t')
            if len(text) <= 4: continue
            try: int(text[0][0])
            except(ValueError): continue
            ra = text[4].replace(' ',':')
            dec = text[5].replace(' ',':')
            name = text[2].strip()
            jys = float(text[9])/1000
            addsrcs.append(a.fit.RadioFixedBody(ra, dec, name=name,
                jys=jys, index=0, mfreq=0.330))
        self.add_srcs(addsrcs)

WENSSFILE = os.path.join(os.path.dirname(__file__), 'wenss.txt')
_wensscat = None

def get_srcs(srcs=None, cutoff=None):
    global _wensscat
    if _wensscat is None:
        _wensscat = WenssCatalog()
        _wensscat.fromfile(WENSSFILE)
    if srcs is None:
        if cutoff is None: srcs = _wensscat.keys()
        else:
            cut, fq = cutoff
            fq = np.array([fq])
            for s in _wensscat.keys(): _wensscat[s].update_jys(fq)
            srcs = [s for s in _wensscat.keys() if _wensscat[s].jys[0] > cut]
    srclist = []
    for s in srcs:
        try: srclist.append(_wensscat[s])
        except(KeyError): pass
    return srclist
