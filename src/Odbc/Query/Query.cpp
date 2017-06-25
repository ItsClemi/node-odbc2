// File: Query.cpp
// 
// node-odbc (odbc interface for NodeJS)
// 
// Copyright 2017 Clemens Susenbeth
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "stdafx.h"
#include "Query.h"

#include "Odbc/Connection/ConnectionPool.h"



using namespace v8;


CQuery::CQuery( const std::shared_ptr< CConnectionPool > pPool )
	: m_pPool( pPool ), m_resultSet( this )
{
#ifdef _DEBUG
	gEnv->pQueryTracker->AddQuery( this );
#endif

	SetState( EQueryState::eExecuteStatement );
}

CQuery::~CQuery( )
{
	assert( m_pConnection == nullptr );
	assert( v8::Isolate::GetCurrent( ) != nullptr );

	m_pPool->FinalizeQuery( );

#ifdef _DEBUG
	gEnv->pQueryTracker->RemoveQuery( this );
#endif

	Isolate::GetCurrent( )->AdjustAmountOfExternalAllocatedMemory( -static_cast< int64_t >( sizeof( CQuery ) ) );
}


void CQuery::ProcessBackground( )
{
	ProcessQuery( );

	if( GetState( ) == EQueryState::eEnd )
	{
		GetStatement()->FreeHandle( );
		m_pPool->PushConnection( m_pConnection );
		
		m_pConnection = nullptr; 
	}
}

void CQuery::ProcessQuery( )
{
	if( GetState( ) == EQueryState::eExecuteStatement )
	{
		if( !ExecuteStatement( ) )
		{
			SetError( );
			return;
		}
	}

	if( GetState( ) == EQueryState::eNeedData )
	{
		if( !GetParamData( ) )
		{
			SetError( );
			return;
		}
	}

	if( GetState( ) == EQueryState::eFetchResult )
	{
		if( !GetResultSet( )->FetchResults( ) )
		{
			SetError( );
			return;
		}

		SetState( EQueryState::eEnd );
	}
}


EForegroundResult CQuery::ProcessForeground( v8::Isolate* isolate )
{
	if( GetState( ) == EQueryState::eNeedData )
	{
		InvokeReadData( isolate );
	}

	if( GetState( ) == EQueryState::eEnd )
	{
		if( HasError( ) )
		{
			GetResultSet( )->Resolve( isolate, m_pError->ConstructErrorObject( isolate ) );
		}
		else
		{
			GetResultSet( )->Resolve( isolate, GetResultSet( )->ConstructResult( isolate ) );
		}

		return EForegroundResult::eDiscard;
	}

	return EForegroundResult::eReschedule;
}



bool CQuery::ExecuteStatement( )
{
	if( !GetStatement( )->AllocHandle( m_pConnection->GetSqlHandle( ) ) )
	{
		return false;
	}

	if( !BindOdbcParameters( ) )
	{
		return false;
	}

	if( m_nQueryTimeout > 0 )
	{
		if( !GetStatement( )->SetStmtAttr( SQL_ATTR_QUERY_TIMEOUT, reinterpret_cast< SQLPOINTER >( m_nQueryTimeout ), SQL_IS_UINTEGER ) )
		{
			return false;
		}
	}

	SQLRETURN sqlRet = GetStatement( )->ExecDirect( m_szQuery );

	if( sqlRet == SQL_NEED_DATA )
	{
		SetState( EQueryState::eNeedData );
		return true;
	}
	else if( sqlRet == SQL_NO_DATA )
	{
		m_bExecNoData = true;
	}
	else if( !SQL_SUCCEEDED( sqlRet ) )
	{
		return false;
	}

	SetState( EQueryState::eFetchResult );

	return true;
}

bool CQuery::BindOdbcParameters( )
{
	SQLUSMALLINT nParam = 1;
	if( m_bReturnValue )
	{
		if( !GetStatement( )->BindParameter( nParam++, SQL_PARAM_OUTPUT, SQL_C_SSHORT, SQL_INTEGER, 0, 0, &m_nReturnValue, 0, &m_nCbReturnValue ) )
		{
			return false;
		}
	}

	for( auto& i : GetQueryParam( )->m_vecParameter )
	{
		if( !GetStatement( )->BindParameter(
			nParam++,
			i.m_nInputOutputType,
			i.m_nValueType,
			i.m_nParameterType,
			i.m_nColumnSize,
			i.m_nDigits,
			i.m_pBuffer,
			i.m_nBufferLen,
			&i.m_strLen_or_IndPtr
		) )
		{
			return false;
		}
	}

	return true;
}

bool CQuery::GetParamData( )
{
	SQLPOINTER pParam = nullptr;

	SQLRETURN sqlRet = GetStatement( )->ParamData( &pParam );

	if( sqlRet == SQL_NEED_DATA )
	{
		m_pActiveStreamParam = static_cast< CBindParam* >( pParam );
		return true;
	}
	else if( !SQL_SUCCEEDED( sqlRet ) )
	{
		return false;
	}

	SetState( EQueryState::eFetchResult );
	return true;
}

void CQuery::InvokeReadData( v8::Isolate* isolate )
{

}

void CQuery::SetError( )
{
	m_pError = GetStatement( )->GetOdbcError( );
	SetState( EQueryState::eEnd );
}

