#pragma once

#include "DataType.h"


class CSqlReal : public CDataType
{
public:
	CSqlReal( );
	virtual ~CSqlReal( );

protected:
	virtual void TransformType( v8::Isolate* isolate, v8::Local<v8::Value> value, SSqlBindParam* pParam ) override;
	virtual bool TransformSqlType( COdbcStatementHandle* pStatement, size_t nColumn, SSqlData* pData ) override;

};