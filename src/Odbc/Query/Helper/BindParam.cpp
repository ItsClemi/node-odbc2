// File: BindParam.cpp
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
#include "BindParam.h"


using namespace v8;



CBindParam::CBindParam( )
{
}

CBindParam::~CBindParam( )
{
	assert( v8::Isolate::GetCurrent( ) != nullptr );
	assert( m_paramRef.IsEmpty( ) );
}

void CBindParam::Dispose( )
{
	if( !m_paramRef.IsEmpty( ) )
	{
		assert( v8::Isolate::GetCurrent( ) != nullptr );
		m_paramRef.Reset( );
	}

	if( m_nParameterType == SQL_WVARCHAR )
	{
		m_data.stringDesc.Dispose( );
	}
	else if( m_nParameterType == SQL_VARBINARY )
	{
		delete[ ] m_data.bufferDesc.m_pBuffer; 						//scalable_free( m_data.bufferDesc.m_pBuffer );
	}
}

void CBindParam::UpdateOutputParam( Isolate* isolate )
{
	HandleScope scope( isolate );

	auto valueRef = node::PersistentToLocal< Value, CopyablePersistentTraits< Value > >( isolate, m_paramRef );
	{
		valueRef = JSValue::ToValue( isolate, m_eOutputType, m_data );
	}
}

bool CBindParam::SetNumeric( Isolate* isolate, Local< Object > value )
{
	HandleScope scope( isolate );
	const auto context = isolate->GetCurrentContext( );

	auto _precision = value->Get( context, Nan::New( "precision" ).ToLocalChecked( ) );
	auto _scale = value->Get( context, Nan::New( "scale" ).ToLocalChecked( ) );
	auto _sign = value->Get( context, Nan::New( "sign" ).ToLocalChecked( ) );
	auto _buffer = value->Get( context, Nan::New( "value" ).ToLocalChecked( ) );

	if( _precision.IsEmpty( ) || _scale.IsEmpty( ) || _sign.IsEmpty( ) || _buffer.IsEmpty( ) )
	{
		return false;
	}

	auto precision = _precision.ToLocalChecked( );
	auto scale = _scale.ToLocalChecked( );
	auto sign = _sign.ToLocalChecked( );
	auto buffer = _buffer.ToLocalChecked( );

	if( !precision->IsUint32( ) || !scale->IsUint32( ) || !sign->IsBoolean( ) || !buffer->IsUint8Array( ) )
	{
		return false;
	}

	auto nPrecision = precision.As< v8::Uint32 >( )->Uint32Value( context ).FromJust( );
	auto nScale = scale.As< v8::Uint32 >( )->Uint32Value( context ).FromJust( );
	auto bSign = sign.As< v8::Boolean >( )->BooleanValue( context ).FromJust( );
	auto contents = buffer.As< v8::Uint8Array >( )->Buffer( )->GetContents( );

	if( nPrecision > 255 || nScale > 127 || contents.ByteLength( ) > SQL_MAX_NUMERIC_LEN )
	{
		return false;
	}

	size_t nDigits = contents.ByteLength( );
	{
		memset( &m_data.sqlNumeric.val, 0, SQL_MAX_NUMERIC_LEN );

		m_data.sqlNumeric.precision = static_cast< SQLCHAR >( nPrecision );
		m_data.sqlNumeric.scale = static_cast< SQLSCHAR >( nScale );
		m_data.sqlNumeric.sign = static_cast< SQLCHAR >( bSign ? 1 : 2 );
		memcpy_s( m_data.sqlNumeric.val, SQL_MAX_NUMERIC_LEN, contents.Data( ), nDigits );
	}

	SetData( SQL_PARAM_INPUT, SQL_C_NUMERIC, SQL_NUMERIC, m_data.sqlNumeric.precision, static_cast< SQLSMALLINT >( nDigits ), &m_data.sqlNumeric, sizeof( SQL_NUMERIC_STRUCT ), sizeof( SQL_NUMERIC_STRUCT ) );

	return true;
}

bool CBindParam::SetOutputParameter( Isolate* isolate, Local< Object > value )
{
	HandleScope scope( isolate );
	const auto context = isolate->GetCurrentContext( );

	auto _paramType = value->Get( context, Nan::New( "paramType" ).ToLocalChecked( ) );
	auto _ref = value->Get( context, Nan::New( "reference" ).ToLocalChecked( ) );
	auto _length = value->Get( context, Nan::New( "length" ).ToLocalChecked( ) );
	auto _precision = value->Get( context, Nan::New( "precision" ).ToLocalChecked( ) );
	auto _scale = value->Get( context, Nan::New( "scale" ).ToLocalChecked( ) );

	if( _paramType.IsEmpty( ) || _ref.IsEmpty( ) || _length.IsEmpty( ) || _precision.IsEmpty( ) || _scale.IsEmpty( ) )
	{
		return false;
	}

	auto paramType = _paramType.ToLocalChecked( );
	auto ref = _ref.ToLocalChecked( );
	auto length = _length.ToLocalChecked( );
	auto precision = _precision.ToLocalChecked( );
	auto scale = _scale.ToLocalChecked( );
	
	if( !paramType->IsUint32( ) || ref->IsUndefined( ) || !length->IsUint32( ) || !precision->IsUint32( ) || !scale->IsUint32( ) )
	{
		return false;
	}


	ESqlType eType = static_cast< ESqlType >( paramType->Uint32Value( context ).FromJust( ) );
	size_t nLength = static_cast< size_t >( length->IntegerValue( context ).FromJust( ) );
	uint32_t nPrecision = static_cast< uint32_t >( precision->Uint32Value( context ).FromJust( ) );
	uint32_t nScale = static_cast< uint32_t >( scale->Uint32Value( context ).FromJust( ) );


	switch( eType )
	{
		case ESqlType::eBit:
		{
			SetPrimitve< SQLCHAR >( SQL_PARAM_OUTPUT, SQL_C_BIT, SQL_BIT, &m_data.bValue );
			break;
		}
		case ESqlType::eTinyint:
		{
			SetPrimitve< int32_t >( SQL_PARAM_OUTPUT, SQL_C_SLONG, SQL_TINYINT, &m_data.nInt32 );
			break;
		}
		case ESqlType::eSmallint:
		{
			SetPrimitve< int32_t >( SQL_PARAM_OUTPUT, SQL_C_SLONG, SQL_SMALLINT, &m_data.nInt32 );
			break;
		}
		case ESqlType::eInt32:
		{
			SetPrimitve< int32_t >( SQL_PARAM_OUTPUT, SQL_C_SLONG, SQL_INTEGER, &m_data.nInt32 );
			break;
		}
		case ESqlType::eBigInt:
		{
			SetPrimitve< int64_t >( SQL_PARAM_OUTPUT, SQL_C_SBIGINT, SQL_BIGINT, &m_data.nInt64 );
			break;
		}
		case ESqlType::eReal:
		{
			SetPrimitve< double >( SQL_PARAM_OUTPUT, SQL_C_DOUBLE, SQL_DOUBLE, &m_data.dNumber );
			break;
		}
		case ESqlType::eChar:
		{
			m_data.stringDesc.Alloc( EStringType::eAnsi, nLength );
			SetData( SQL_PARAM_OUTPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, m_data.stringDesc.data.pString, nLength, nLength );
			break;
		}
		case ESqlType::eNChar:
		{
			m_data.stringDesc.Alloc( EStringType::eUnicode, nLength );
			nLength *= sizeof( wchar_t );
			SetData( SQL_PARAM_OUTPUT, SQL_C_WCHAR, SQL_WCHAR, 0, 0, m_data.stringDesc.data.pWString, nLength, nLength );
			break;
		}
		case ESqlType::eVarChar:
		{
			m_data.stringDesc.Alloc( EStringType::eAnsi, nLength );
			SetData( SQL_PARAM_OUTPUT, SQL_C_CHAR, SQL_VARCHAR, 0, 0, m_data.stringDesc.data.pString, nLength, nLength );
			break;
		}
		case ESqlType::eNVarChar:
		{
			m_data.stringDesc.Alloc( EStringType::eUnicode, nLength );
			nLength *= sizeof( wchar_t );
			SetData( SQL_PARAM_OUTPUT, SQL_C_WCHAR, SQL_WVARCHAR, 0, 0, m_data.stringDesc.data.pWString, nLength, nLength );
			break;
		}
		case ESqlType::eBinary:
		{
			__debugbreak( );
			break;
		}
		case ESqlType::eVarBinary:
		{
			__debugbreak( );
			break;
		}
		case ESqlType::eDate:
		{
			SetPrimitve< SQL_DATE_STRUCT >( SQL_PARAM_OUTPUT, SQL_C_TYPE_DATE, SQL_TYPE_DATE, &m_data.sqlDate );
			break;
		}
		case ESqlType::eTimestamp: 
		{
			SetData( SQL_PARAM_OUTPUT, SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP, 0, static_cast< SQLSMALLINT >( nScale ), &m_data.sqlDate, sizeof( SQL_TIMESTAMP_STRUCT ), sizeof( SQL_TIMESTAMP_STRUCT ) );
			break;
		}
		case ESqlType::eNumeric:
		{
			SetData( SQL_PARAM_OUTPUT, SQL_C_NUMERIC, SQL_NUMERIC, static_cast< SQLUINTEGER  >( nPrecision ), static_cast< SQLSMALLINT >( nScale ), &m_data.sqlNumeric, sizeof( SQL_NUMERIC_STRUCT ), sizeof( SQL_NUMERIC_STRUCT ) );
			break;
		}
		default:
		{
			return false;
		}
	}

	m_eOutputType = eType;
	m_paramRef.Reset( isolate, ref );

	return true;
}
