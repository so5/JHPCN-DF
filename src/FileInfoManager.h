/*
 * JHPCN-DF - Data compression library based on
 *            Jointed Hierarchical Precision Compression Number Data Format
 *
 * Copyright (c) 2014-2015 Advanced Institute for Computational Science, RIKEN.
 * All rights reserved.
 *
 */

// @file FileInfoManager.h

#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H
#include <string>
#include <iostream>
#include <map>
#include <stdio.h>
namespace JHPCNDF
{
  class FileInfo
  {
    private:
      FileInfo();
      FileInfo (const FileInfo &);
      FileInfo & operator = (const FileInfo &);
    public:
      FileInfo(const std::string& arg_filename_upper, const std::string& arg_filename_lower, const char* mode, const size_t& arg_buffer_size, const std::string& arg_compression_method)
        :filename_upper(arg_filename_upper),filename_lower(arg_filename_lower), fp_upper(NULL), fp_lower(NULL), buffer_size(arg_buffer_size), compression_method(arg_compression_method)
      {
        this->fp_upper=::fopen(filename_upper.c_str(), mode);
        this->filename_upper=filename_upper;
        if(filename_lower!="")
        {
          this->fp_lower=::fopen(filename_lower.c_str(), mode);
          this->filename_lower=filename_lower;
        }
      }
      ~FileInfo()
      {
        if(fp_upper !=NULL)
        {
          fclose(fp_upper);
          fp_upper=NULL;
        }
        if(fp_lower !=NULL)
        {
          fclose(fp_lower);
          fp_lower=NULL;
        }
      }
      FILE* fp_upper;
      FILE* fp_lower;
      std::string filename_upper;
      std::string filename_lower;
      size_t buffer_size;
      std::string compression_method;
  };
  class FileInfoManager
  {
    private:
    FileInfoManager() 
    {
    }
    ~FileInfoManager()
    {
      for(std::map<int, FileInfo*>::iterator it =table.begin(); it!= table.end(); ++it)
      {
        delete it->second;
        table.erase(it);
      }
    }
    FileInfoManager(const FileInfoManager& obj);
    FileInfoManager& operator=(const FileInfoManager& obj);

    public:
    static FileInfoManager& GetInstance()
    {
      static FileInfoManager instance;
      return instance;
    }

    //@brief FilePointerの管理テーブルに新しいエントリを追加する。
    //@param filename_upper     上位bit側のデータを出力するファイルの名前
    //@param filename_lower     下位bit側のデータを出力するファイルの名前(下位bit側のデータを出力しない時は""を指定する
    //@param key                テーブルに登録する時のkey
    //@param mode               ファイルopen時のモード
    //@param compression_method 圧縮方式の指定
    //@param buff_size          gzip圧縮時のIOバッファサイズ
    //@ret   -100以下           エラーコード
    //@ret   0以上              作成されたエントリに対応するkey
    //
    //keyを明示的に指定しなかった場合は、100以上で空いている値を探して使う
    //指定された場合はその値をkeyとして登録する(Fortran Interfaceから装置番号として指定されることを想定している）
    //
    int create_new_entry(const std::string& filename_upper, const std::string& filename_lower="", int key=-1, const char * mode="w+b", const std::string& compression_method="stdio", const size_t& buffer_size=32768)
    {
      // 名前の重複チェック
      for(std::map<int, FileInfo*>::iterator it=table.begin(); it!=table.end(); ++it)
      {
        if(filename_upper == it->second->filename_upper)
        {
          std::cerr << filename_upper <<" is already opend"<<std::endl;
          return -100;
        }
      }

      // keyの重複チェック
      if(table.find(key)!=table.end())
      {
        std::cerr <<"key number "<<key <<" is already opend"<<std::endl;
        return -200;
      }

      // 名前の有効性チェック
      if(filename_upper.empty())
      {
        std::cerr <<"Invalid file name"<<std::endl;
        return -300;
      }
      //
      if(filename_upper == filename_lower)
      {
        std::cerr <<"filename for upper bits and lower bits must be different"<<std::endl;
        return -400;
      }

      // modeの正当性チェック
      std::string str_mode(mode);
      if (str_mode != "r"   &&
          str_mode != "r+"  &&
          str_mode != "w"   &&
          str_mode != "w+"  &&
          str_mode != "a"   &&
          str_mode != "a+"  &&
          str_mode != "rb"  &&
          str_mode != "r+b" &&
          str_mode != "wb"  &&
          str_mode != "w+b" &&
          str_mode != "ab"  &&
          str_mode != "a+b")
      {
        std::cerr <<"Invalid mode specified"<<std::endl;
        return -500;
      }

      FileInfo* tmp = new FileInfo(filename_upper, filename_lower, mode, buffer_size, compression_method);

      if(key < 0)
      {
        key=100;
        while(table.find(key) != table.end())
        {
          key++;
        }
      }
      table.insert(std::make_pair(key,tmp));
      return key;
    }

    //@brief 指定されたkeyに対応するエントリをテーブルから削除する
    bool destroy_entry(const int& key)
    {
      FileInfo* tmp=get_entry(key);
      if(tmp==NULL)
      {
        return false;
      }
      delete tmp;
      table.erase(key);
      return true;
    }

    //@brief 指定されたkeyに対応する上位ビット側のファイルポインタを返す
    FILE* get_upper_file_pointer(const int& key)
    {
      FileInfo* tmp=get_entry(key);
      if(tmp==NULL)
      {
        return NULL;
      }
      return tmp->fp_upper;
    }

    //@brief 指定されたkeyに対応する下位ビット側のファイルポインタを返す
    FILE* get_lower_file_pointer(const int& key)
    {
      FileInfo* tmp=get_entry(key);
      if(tmp==NULL)
      {
        return NULL;
      }
      return tmp->fp_lower;
    }

    //@brief 登録済の全てのエントリを削除する
    void destroy_all(void)
    {
      for(std::map<int, FileInfo*>::iterator it =table.begin(); it!= table.end(); ++it)
      {
        delete it->second;
        table.erase(it);
      }
    }

    //@brief 指定されたkeyに対応するファイルへのIOバッファサイズを取得する
    const size_t get_buff_size(const int& key)
    {
      FileInfo*tmp=get_entry(key);
      if(tmp==NULL) 
      {
        std::cerr<<"Invalie key("<<key<<")"<<std::endl;
        return 1;
      }
      return tmp->buffer_size;
    }

    //@ brief 指定されたkeyに対応するファイルへの圧縮方式を取得する
    const std::string get_compression_method(const int& key)
    {
      FileInfo*tmp=get_entry(key);
      if(tmp==NULL) 
      {
        std::cerr<<"Invalie key("<<key<<")"<<std::endl;
        return "";
      }
      return tmp->compression_method;
    }

    private:
    //@brief 指定されたkeyに対応するFileInfo構造体へのポインタを返す
    //
    //keyがテーブルに含まれていなかった場合はstderrへメッセージを出力して
    //NULLを返す（呼び出し側でNULLかどうかのチェックをすること）
    FileInfo* get_entry(const int& key)
    {
      std::map<int, FileInfo*>::iterator it=table.find(key);
      if(it==table.end())
      {
        std::cerr <<"key number "<<key <<" is not found"<<std::endl;
        return NULL;
      }
      return it->second;
    }
    std::map<int, FileInfo*> table;
  };
}//end of namespace JHPCNDF
#endif
