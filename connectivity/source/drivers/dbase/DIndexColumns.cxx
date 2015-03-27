/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 *************************************************************/



// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_connectivity.hxx"
#include "dbase/DIndexColumns.hxx"
#include "dbase/DTable.hxx"
#include "connectivity/sdbcx/VIndexColumn.hxx"
#include <comphelper/types.hxx>
#include <comphelper/property.hxx>
#include <connectivity/dbexception.hxx>

using namespace ::comphelper;

using namespace connectivity::dbase;
using namespace connectivity;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::sdbcx;
using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::container;


sdbcx::ObjectType ODbaseIndexColumns::createObject(const ::rtl::OUString& _rName)
{
	const ODbaseTable* pTable = m_pIndex->getTable();

	::vos::ORef<OSQLColumns> aCols = pTable->getTableColumns();
	OSQLColumns::Vector::const_iterator aIter = find(aCols->get().begin(),aCols->get().end(),_rName,::comphelper::UStringMixEqual(isCaseSensitive()));

	Reference< XPropertySet > xCol;
	if(aIter != aCols->get().end())
		xCol = *aIter;

	if(!xCol.is())
		return sdbcx::ObjectType();

	sdbcx::ObjectType xRet = new sdbcx::OIndexColumn(sal_True,_rName
													,getString(xCol->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_TYPENAME)))
													,::rtl::OUString()
													,getINT32(xCol->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_ISNULLABLE)))
													,getINT32(xCol->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_PRECISION)))
													,getINT32(xCol->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_SCALE)))
													,getINT32(xCol->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_TYPE)))
													,sal_False
													,sal_False
													,sal_False
													,pTable->getConnection()->getMetaData()->supportsMixedCaseQuotedIdentifiers());

	return xRet;
}

// -------------------------------------------------------------------------
void ODbaseIndexColumns::impl_refresh() throw(RuntimeException)
{
	m_pIndex->refreshColumns();
}
// -------------------------------------------------------------------------
Reference< XPropertySet > ODbaseIndexColumns::createDescriptor()
{
	return new sdbcx::OIndexColumn(m_pIndex->getTable()->getConnection()->getMetaData()->supportsMixedCaseQuotedIdentifiers());
}
// -------------------------------------------------------------------------
sdbcx::ObjectType ODbaseIndexColumns::appendObject( const ::rtl::OUString& /*_rForName*/, const Reference< XPropertySet >& descriptor )
{
    return cloneDescriptor( descriptor );
}
// -----------------------------------------------------------------------------


