import * as stream from "stream";
import * as fs from "fs";
export declare const enum eSqlType {
    eNull = 0,
    eBit = 1,
    eTinyint = 2,
    eSmallint = 3,
    eInt32 = 4,
    eBigInt = 5,
    eReal = 6,
    eChar = 7,
    eNChar = 8,
    eVarChar = 9,
    eNVarChar = 10,
    eBinary = 11,
    eVarBinary = 12,
    eDate = 13,
    eTimestamp = 14,
    eNumeric = 15,
    eLongVarChar = 16,
    eLongNVarChar = 17,
    eLongVarBinary = 18,
    eSqlOutputVar = 19,
}
export declare type SqlComplexType = {
    readonly _typeId: number;
};
export declare type SqlStream = SqlComplexType & {
    type: eSqlType;
    stream: stream.Readable | stream.Writable;
    length: number;
};
export declare type SqlNumeric = SqlComplexType & {
    precision: number;
    scale: number;
    sign: boolean;
    value: Uint8Array;
};
export declare type SqlTimestamp = SqlComplexType & {
    date: Date;
};
export declare type SqlOutputParameter = SqlComplexType & {
    reference: SqlTypes | Uint8Array;
    paramType: eSqlType;
    length: number;
    precision: number;
    scale: number;
};
export declare type SqlError = {
    readonly message: string;
    readonly sqlState: string;
    readonly code: number;
};
export declare type SqlColumnMetaData = {
    readonly name: string;
    readonly size: number;
    readonly dataType: string;
    readonly digits: number;
    readonly nullable: boolean;
};
export declare type SqlResultExtension = {
    readonly $sqlReturnValue?: number;
    readonly $sqlMetaData?: Array<SqlColumnMetaData>;
};
export declare type SqlResult = SqlResultExtension & Partial<any>;
export declare type SqlPartialResult<T> = SqlResultExtension & Partial<T>;
export declare type SqlResultArray = SqlResultExtension & Array<any>;
export declare type SqlPartialResultArray<T> = SqlResultExtension & Array<T>;
export declare type SqlResultTypes = SqlResult | SqlResultArray;
export declare type SqlPartialResultTypes<T> = SqlPartialResult<T> & SqlPartialResultArray<T>;
export declare type SqlTypes = null | string | boolean | number | Date | Buffer | SqlStream | SqlNumeric | SqlTimestamp | SqlOutputParameter;
export interface IResilienceStrategy {
    retries: number;
    errorCodes: Array<number>;
}
export declare type ConnectionInfo = {
    driverName: string;
    driverVersion: string;
    databaseName: string;
    odbcVersion: string;
    dbmsName: string;
    internalServerType: number;
    odbcConnectionString: string;
    resilienceStrategy: IResilienceStrategy;
};
export declare type ConnectionProps = {
    enableMssqlMars?: boolean;
    poolSize?: number;
};
export declare const enum eFetchMode {
    eSingle = 0,
    eArray = 1,
}
export declare var enableValidation: boolean;
export declare class Connection {
    private _connection;
    constructor(advancedProps?: ConnectionProps);
    connect(connectionString: string, connectionTimeout?: number): Connection;
    disconnect(cb: () => void): void;
    prepareQuery(query: string, ...args: (SqlTypes)[]): ISqlQuery;
    executeQuery<T>(eFetchOperation: eFetchMode, cb: (result: SqlPartialResultTypes<T>, error: SqlError) => void, query: string, ...args: (SqlTypes)[]): void;
    getInfo(): ConnectionInfo;
}
export interface ISqlQuery {
    enableReturnValue(): ISqlQuery;
    enableMetaData(): ISqlQuery;
    setQueryTimeout(timeout: number): ISqlQuery;
    toSingle<T>(cb: (result: SqlPartialResult<T>, error: SqlError) => void): void;
    toSingle<T>(): Promise<SqlPartialResult<T>>;
    toArray<T>(cb: (result: SqlPartialResultArray<T>, error: SqlError) => void): void;
    toArray<T>(): Promise<SqlPartialResultArray<T>>;
}
export interface ISqlQueryEx extends ISqlQuery {
    setPromiseInfo(resolve: any, reject: any): void;
}
export declare function makeInputStream(type: eSqlType.eLongNVarChar | eSqlType.eLongVarBinary, stream: fs.ReadStream | stream.Readable, length: number): SqlStream;
export declare function makeNumeric(precision: number, scale: number, sign: boolean, value: Uint8Array): SqlNumeric;
export declare function makeTimestamp(date: Date): SqlTimestamp;
export declare const SqlOutput: {
    asBitOutput(reference: boolean): SqlOutputParameter;
    asTinyint(reference: number): SqlOutputParameter;
    asSmallint(reference: number): SqlOutputParameter;
    asInt(reference: number): SqlOutputParameter;
    asBigInt(reference: number): SqlOutputParameter;
    asReal(reference: number): SqlOutputParameter;
    asChar(reference: string, length: number): SqlOutputParameter;
    asNChar(reference: string, length: number): SqlOutputParameter;
    asVarChar(reference: string, length: number): SqlOutputParameter;
    asNVarChar(reference: string, length: number): SqlOutputParameter;
    asBinary(reference: Uint8Array, length: number): SqlOutputParameter;
    asVarBinary(reference: Uint8Array, length: number): SqlOutputParameter;
    asDate(reference: Date, scale: number): SqlOutputParameter;
    asTimestamp(reference: Date, scale: number): SqlOutputParameter;
    asNumeric(reference: SqlNumeric, precision: number, scale: number): SqlOutputParameter;
};
export interface IJSBridge {
    setWriteStreamInitializer(cb: (targetStream: stream.Readable, query: ISqlQueryEx) => void): void;
    setPromiseInitializer<T>(cb: (query: ISqlQueryEx) => T): void;
    setReadStreamInitializer(cb: (query: ISqlQueryEx, column: number) => stream.Readable): void;
    processNextChunk(query: ISqlQueryEx, chunk: Int8Array, cb: (error) => void): void;
    requestNextChunk(query: ISqlQueryEx, column: number, cb: (chunk: Int8Array) => void): Int8Array;
}
export declare function getJSBridge(): IJSBridge;
