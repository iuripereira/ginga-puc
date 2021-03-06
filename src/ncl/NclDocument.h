/* Copyright (C) 2006-2017 PUC-Rio/Laboratorio TeleMidia

This file is part of Ginga (Ginga-NCL).

Ginga is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Ginga is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
License for more details.

You should have received a copy of the GNU General Public License
along with Ginga.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef NCLDOCUMENT_H_
#define NCLDOCUMENT_H_

#include "Rule.h"
#include "RuleBase.h"

#include "ContextNode.h"
#include "Node.h"
#include "NodeEntity.h"

#include "Connector.h"
#include "ConnectorBase.h"

#include "GenericDescriptor.h"
#include "DescriptorBase.h"

#include "LayoutRegion.h"
#include "RegionBase.h"

#include "Transition.h"
#include "TransitionBase.h"

#include "Meta.h"
#include "Metadata.h"

#include "Base.h"

GINGA_NCL_BEGIN

class NclDocument
{
private:
  ContextNode *body;
  ConnectorBase *connectorBase;
  DescriptorBase *descriptorBase;
  map<string, NclDocument *> documentAliases;
  vector<NclDocument *> documentBase;
  map<string, NclDocument *> documentLocations;
  string id;
  map<int, RegionBase *> regionBases;
  RuleBase *ruleBase;
  TransitionBase *transitionBase;
  vector<Meta *> metainformationList;
  vector<Metadata *> metadataList;

  string docLocation;
  NclDocument *parentDocument;

public:
  NclDocument (const string &id, const string &docLocation);
  ~NclDocument ();

  NclDocument *getParentDocument ();
  void setParentDocument (NclDocument *parentDocument);
  string getDocumentPerspective ();

  string getDocumentLocation ();

  bool addDocument (NclDocument *document, const string &alias,
                    const string &location);
  void clearDocument ();
  Connector *getConnector (const string &connectorId);
  ConnectorBase *getConnectorBase ();
  Transition *getTransition (const string &transitionId);
  TransitionBase *getTransitionBase ();
  GenericDescriptor *getDescriptor (const string &descriptorId);
  DescriptorBase *getDescriptorBase ();
  NclDocument *getDocument (const string &documentId);
  string getDocumentAlias (NclDocument *document);
  ContextNode *getBody ();
  string getDocumentLocation (NclDocument *document);
  vector<NclDocument *> *getDocuments ();
  string getId ();

private:
  Node *getNodeLocally (const string &nodeId);

public:
  Node *getNode (const string &nodeId);
  LayoutRegion *getRegion (const string &regionId);

private:
  LayoutRegion *getRegion (const string &regionId, RegionBase *regionBase);

public:
  RegionBase *getRegionBase (int devClass);
  RegionBase *getRegionBase (const string &regionBaseId);
  map<int, RegionBase *> *getRegionBases ();
  Rule *getRule (const string &ruleId);
  RuleBase *getRuleBase ();
  bool removeDocument (NclDocument *document);
  void setConnectorBase (ConnectorBase *connectorBase);
  void setTransitionBase (TransitionBase *transitionBase);
  void setDescriptorBase (DescriptorBase *descriptorBase);
  void setDocumentAlias (NclDocument *document, const string &alias);
  void setBody (ContextNode *node);
  void setDocumentLocation (NclDocument *document, const string &location);
  void setId (const string &id);
  void addRegionBase (RegionBase *regionBase);
  void setRuleBase (RuleBase *ruleBase);
  void addMetainformation (Meta *meta);
  void addMetadata (Metadata *metadata);
  vector<Meta *> *getMetainformation ();
  vector<Metadata *> *getMetadata ();
  void removeRegionBase (const string &regionBaseId);
  void removeMetainformation (Meta *meta);
  void removeMetadata (Metadata *metadata);
  void removeAllMetainformation ();
  void removeAllMetadata ();
};

GINGA_NCL_END

#endif /*NCLDOCUMENT_H_*/
