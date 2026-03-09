#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Excel to SQLite Converter
Converts Excel configuration files to SQLite database for game server
"""

import os
import sys
import sqlite3
import argparse
from openpyxl import load_workbook

class ExcelToSQLiteConverter:
    def __init__(self, db_path):
        self.db_path = db_path
        self.conn = None
        self.cursor = None
        
    def connect(self):
        """Connect to SQLite database"""
        self.conn = sqlite3.connect(self.db_path)
        self.cursor = self.conn.cursor()
        print(f"Connected to database: {self.db_path}")
        
    def close(self):
        """Close database connection"""
        if self.conn:
            self.conn.commit()
            self.conn.close()
            print("Database connection closed")
            
    def get_sql_type(self, value):
        """Determine SQL type from Python value"""
        if isinstance(value, int):
            return "INTEGER"
        elif isinstance(value, float):
            return "REAL"
        elif isinstance(value, bool):
            return "INTEGER"
        else:
            return "TEXT"
            
    def convert_value(self, value):
        """Convert Excel value to appropriate Python type"""
        if value is None:
            return None
        if isinstance(value, str):
            value = value.strip()
            if value == "":
                return None
            try:
                if "." in value:
                    return float(value)
                return int(value)
            except ValueError:
                return value
        return value
        
    def convert_sheet(self, sheet, table_name=None):
        """Convert a single Excel sheet to SQLite table"""
        if table_name is None:
            table_name = sheet.title
            
        print(f"Converting sheet: {sheet.title} -> {table_name}")
        
        rows = list(sheet.rows)
        if len(rows) < 2:
            print(f"  Skipping empty sheet: {sheet.title}")
            return False
            
        header_row = rows[0]
        type_row = rows[1]
        data_rows = rows[2:]
        
        columns = []
        column_names = []
        
        for i, (header_cell, type_cell) in enumerate(zip(header_row, type_row)):
            col_name = str(header_cell.value).strip() if header_cell.value else f"col_{i}"
            col_type = str(type_cell.value).strip().upper() if type_cell.value else "TEXT"
            
            if col_type == "INT" or col_type == "INTEGER":
                col_type = "INTEGER"
            elif col_type == "FLOAT" or col_type == "REAL" or col_type == "DOUBLE":
                col_type = "REAL"
            elif col_type == "BOOL" or col_type == "BOOLEAN":
                col_type = "INTEGER"
            else:
                col_type = "TEXT"
                
            columns.append((col_name, col_type))
            column_names.append(col_name)
            
        if not columns:
            print(f"  No columns found in sheet: {sheet.title}")
            return False
            
        self.cursor.execute(f"DROP TABLE IF EXISTS {table_name}")
        
        create_sql = f"CREATE TABLE {table_name} ("
        create_sql += ", ".join([f"{name} {type}" for name, type in columns])
        create_sql += ")"
        
        self.cursor.execute(create_sql)
        
        insert_sql = f"INSERT INTO {table_name} ("
        insert_sql += ", ".join(column_names)
        insert_sql += ") VALUES ("
        insert_sql += ", ".join(["?" for _ in column_names])
        insert_sql += ")"
        
        row_count = 0
        for row in data_rows:
            values = []
            for i, cell in enumerate(row):
                if i < len(columns):
                    value = self.convert_value(cell.value)
                    values.append(value)
                    
            if any(v is not None for v in values):
                self.cursor.execute(insert_sql, values)
                row_count += 1
                
        print(f"  Inserted {row_count} rows into {table_name}")
        return True
        
    def convert_excel(self, excel_path, sheets=None):
        """Convert Excel file to SQLite database"""
        print(f"Loading Excel file: {excel_path}")
        
        wb = load_workbook(excel_path, read_only=True, data_only=True)
        
        converted_count = 0
        for sheet in wb.worksheets:
            if sheets and sheet.title not in sheets:
                continue
                
            if sheet.title.startswith("#") or sheet.title.startswith("_"):
                print(f"  Skipping sheet: {sheet.title}")
                continue
                
            if self.convert_sheet(sheet):
                converted_count += 1
                
        wb.close()
        print(f"Converted {converted_count} sheets from {excel_path}")
        return converted_count
        
    def convert_directory(self, dir_path, pattern="*.xlsx"):
        """Convert all Excel files in a directory"""
        import glob
        
        files = glob.glob(os.path.join(dir_path, pattern))
        total_count = 0
        
        for file_path in files:
            if self.convert_excel(file_path):
                total_count += 1
                
        print(f"Converted {total_count} Excel files from {dir_path}")
        return total_count

def main():
    parser = argparse.ArgumentParser(description="Convert Excel files to SQLite database")
    parser.add_argument("-o", "--output", required=True, help="Output SQLite database path")
    parser.add_argument("-i", "--input", help="Input Excel file or directory")
    parser.add_argument("-d", "--dir", help="Input directory containing Excel files")
    parser.add_argument("-s", "--sheets", nargs="+", help="Specific sheets to convert")
    parser.add_argument("-p", "--pattern", default="*.xlsx", help="File pattern for directory mode")
    
    args = parser.parse_args()
    
    converter = ExcelToSQLiteConverter(args.output)
    converter.connect()
    
    try:
        if args.input:
            if os.path.isdir(args.input):
                converter.convert_directory(args.input, args.pattern)
            else:
                converter.convert_excel(args.input, args.sheets)
        elif args.dir:
            converter.convert_directory(args.dir, args.pattern)
        else:
            print("No input specified. Use -i or -d option.")
            sys.exit(1)
    finally:
        converter.close()

if __name__ == "__main__":
    main()
