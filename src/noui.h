// Copyright (c) 2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NOUI_H
#define BITCOIN_NOUI_H

/** Whether the node runs without a GUI (bitblocksd): no one can answer questions at startup. */
extern bool fNoUI;

extern void noui_connect();

#endif // BITCOIN_NOUI_H
