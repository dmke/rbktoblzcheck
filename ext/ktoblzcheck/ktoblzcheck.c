/***************************************************************************
*
*  (c) 2004 Sascha Loetz (sloetz ( a t ) web.de). All rights reserved.
*
*
*  Redistribution and use in source and binary forms of this software, with
*  or without modification, are permitted provided that the following
*  conditions are met:
*
*  1. Redistributions of source code must retain any existing copyright
*     notice, and this entire permission notice in its entirety,
*     including the disclaimer of warranties.
*
*  2. Redistributions in binary form must reproduce all prior and current
*     copyright notices, this list of conditions, and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*
*  3. The name of any author may not be used to endorse or promote
*     products derived from this software without their specific prior
*     written permission.
*
*  ALTERNATIVELY, this product may be distributed under the terms of the
*  GNU General Public License, in which case the provisions of the GNU
*  GPL are required INSTEAD OF the above restrictions.  (This clause is
*  necessary due to a potential conflict between the GNU GPL and the
*  restrictions contained in a BSD-style copyright.)
*
*  THIS SOFTWARE IS PROVIDED AS IS'' AND ANY EXPRESS OR IMPLIED
*  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
*  IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
*  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
*  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
*  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
*  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
*  DAMAGE.
*************************************************************************
*
* This file is part of the ktoblzcheck package available as a gem
*
* $Id: ktoblzcheck.c,v 1.1 2004/10/28 19:10:19 vdpsoftware Exp $
*
*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ruby.h>
#include <ktoblzcheck.h>
#include <iban.h>

static VALUE g_error;
static VALUE g_ktoblzcheck;
static VALUE g_ktoblzcheck_iban;
static AccountNumberCheck* g_anc = NULL;
static IbanCheck* g_ic = NULL;

#ifndef RUBY_19
#ifndef RSTRING_PTR
#define RSTRING_PTR(v) (RSTRING(v)->ptr)
#endif
#endif

/*
 * call-seq:
 *    kbc.close -> nil
 *
 * Closes the KtoBlzCheck object's handle for `libktoblzcheck`. Must be called
 * to prevent resource leaks if the constructor is not used with block syntax.
 */
static VALUE
close_anc(VALUE self) {
  if (NULL != g_anc) {
    AccountNumberCheck_delete(g_anc);
  }
  return Qnil;
}

static VALUE
close_ic(VALUE self) {
  if (NULL != g_ic) {
    IbanCheck_free(g_ic);
  }
  return Qnil;
}

/*
 * call-seq:
 *    KtoBlzCheck.new([datapath]) {|kbc| block }  -> nil
 *    KtoBlzCheck.new([datapath])                 -> kbc
 *
 * Constructs a new KtoBlzCheck object, returns it or passes it to an optional
 * code block. If no block is provided, the user must call `kbc.close` to
 * ensure that all resources are freed after usage.
 *
 * If a parameter is passed to new it is interpreted as the path to the data
 * file to be used by `libktoblzcheck`. When the parameter is omitted, the
 * default file of `libktoblzcheck` is used  (usually
 * `/usr/[local/]share/[lib]ktoblzcheck[*]/bankdata.txt`).
 *
 * A `nil` value passed as parameter is ignored.
 */
static VALUE
init_anc(int argc, VALUE *argv, VALUE self)
{
  VALUE dp = Qnil;

  rb_scan_args(argc, argv, "01", &dp);

  if (Qnil == dp) {
    /* no parameter given */
    g_anc = AccountNumberCheck_new();
  } else {
    /* a path to a different data path was passed to method */
    Check_Type(dp, T_STRING);

    /*
     * The libktoblzcheck constructor writes an error message to stderr if the
     * given data filen can not be accessed :(
     * Additionally, we don't get a proper return value which could be used to
     * test for success (it's a C++ constructor that we are calling). Even a
     * successful intialization (i.e. we get a `pointer != NULL` from
     * `AccountNumberCheck_new_file` doesn't mean that the opened file is a
     * valid data file).
     *
     * We do basic access checking ourselves and hope for the best :)
     */

    if (0 != access(RSTRING_PTR(dp), R_OK)) {
      rb_raise(g_error, "Can't access file %s", RSTRING_PTR(dp));
    }
    g_anc = AccountNumberCheck_new_file(RSTRING_PTR(dp));
  }

  /* did we successfully obtain an AccountNumberCheck handle? */

  if (NULL == g_anc) {
    rb_raise(g_error, "Couldn't initialize libktoblzcheck");
    return Qnil;
  }

  if (rb_block_given_p()) {
    return rb_ensure(rb_yield, self, close_anc, self);
  } else {
    return self;
  }
}

static VALUE
init_ic(int argc, VALUE *argv, VALUE self)
{
  VALUE dp = Qnil;

  rb_scan_args(argc, argv, "01", &dp);

  if (Qnil == dp) {
    /* no parameter given */
    g_ic = IbanCheck_new(NULL);
  } else {
    /* a path to a different data path was passed to method */
    Check_Type(dp, T_STRING);

    if (0 != access(RSTRING_PTR(dp), R_OK)) {
      rb_raise(g_error, "Can't access file %s", RSTRING_PTR(dp));
    }
    g_ic = IbanCheck_new(RSTRING_PTR(dp));
  }

  /* did we successfully obtain an AccountNumberCheck handle? */
  if (NULL == g_ic) {
    rb_raise(g_error, "Couldn't initialize libktoblzcheck");
    return Qnil;
  }

  if (rb_block_given_p()) {
    return rb_ensure(rb_yield, self, close_ic, self);
  } else {
    return self;
  }
}

/*
 * call-seq:
 *    kbc.check(bank_code, account_no) -> fixnum
 *
 * Checks if `bank_code` and `account_no` form a valid combination. The
 * returned Fixnum indicates the result. The following constants can be
 * used test the result:
 *
 *    KtoBlzCheck::OK             #=> valid combination of bank code and account number
 *    KtoBlzCheck::ERROR          #=> !OK
 *    KtoBlzCheck::UNKNOWN        #=> no verification possible for unknown reason
 *    KtoBlzCheck::BANK_NOT_KNOWN #=> bank code not known
 *
 * If `bank_code` and `account_no` are not of type String, a TypeError is raised.
 */
static VALUE
check_acn(VALUE self, VALUE blz, VALUE account)
{
  AccountNumberCheck_Result res;
  Check_Type(blz, T_STRING);
  Check_Type(account, T_STRING);

  res = AccountNumberCheck_check(g_anc, RSTRING_PTR(blz), RSTRING_PTR(account));
  return INT2FIX(res);
}

static VALUE
check_ic(VALUE self, VALUE iban, VALUE country)
{
  IbanCheck_Result res;
  Check_Type(iban, T_STRING);
  Check_Type(country, T_STRING);

  res = IbanCheck_check_str(g_ic, RSTRING_PTR(iban), RSTRING_PTR(country));
  return INT2FIX(res);
}

/*
 * call-seq:
 *    kbc.num_records -> fixnum
 *
 * Returns the number of entries in the currently used data file for
 * `libktoblzcheck`.
 */
static VALUE
num_records(VALUE self)
{
  return INT2FIX(AccountNumberCheck_bankCount(g_anc));
}

/*
 * call-seq:
 *    kbc.find(bank_code) -> array
 *
 * Looks up bank name and bank location (city) for the bank identified by
 * `bank_code`.
 *
 * Returns an array with the bank's name (index 0) and the bank's location
 * (index 1). The returned array is empty if no bank is found for the given
 * bank code.
 */
static VALUE
find_info(VALUE self, VALUE blz)
{
  VALUE ret = rb_ary_new2(2);
  const AccountNumberCheck_Record* cr;

  Check_Type(blz, T_STRING);

  cr = AccountNumberCheck_findBank(g_anc, RSTRING_PTR(blz));

  if (NULL != cr) {
    rb_ary_push(ret, rb_str_new2(AccountNumberCheck_Record_bankName(cr)));
    rb_ary_push(ret, rb_str_new2(AccountNumberCheck_Record_location(cr)));
  }

  return ret;
}

/*
 * call-seq:
 *    KtoBlzCheck.bankdata_dir -> string
 *
 * Returns the directory where the bankdata file is stored, usually
 * `/usr/[local/]share/[lib]ktoblzcheck[*]/`.
 */
static VALUE
bankdata_dir()
{
  return rb_str_new2(AccountNumberCheck_bankdata_dir());
}

/*
 * call-seq:
 *    KtoBlzCheck.encoding -> string
 *
 * Returns the character encoding that is used when strings are returned. So
 * far this has been `ISO-8859-15` (up to and including version 1.11), but at
 * some point in the future it might change into `UTF-8`.
 *
 * For Ruby scripts, this value has little to no relevance, except you want to
 * mess with `KtoBlzCheck.bankdata_dir`.
 */
static VALUE
encoding()
{
  return rb_str_new2(AccountNumberCheck_stringEncoding());
}

/*
 * Document-class: KtoBlzCheck
 *
 * A collection of methods around German bank account checks.
 */

/*
 * Document-class: KtoBlzCheck::Error
 *
 * Error thrown, when the underlying `libktoblzcheck` fails to complete a task.
 */

/*
 * Ruby extension stuff
 */
void
Init_ktoblzcheck()
{
  g_ktoblzcheck = rb_define_class("KtoBlzCheck", rb_cObject);
  g_error = rb_define_class_under(g_ktoblzcheck, "Error", rb_eStandardError);

  rb_define_singleton_method(g_ktoblzcheck, "bankdata_dir", bankdata_dir, 0);
  rb_define_singleton_method(g_ktoblzcheck, "encoding",     encoding,     0);

  rb_define_const(g_ktoblzcheck,  "VERSION",        rb_str_new2(AccountNumberCheck_libraryVersion()));

  rb_define_const(g_ktoblzcheck,  "OK",             INT2FIX(0));
  rb_define_const(g_ktoblzcheck,  "UNKNOWN",        INT2FIX(1));
  rb_define_const(g_ktoblzcheck,  "ERROR",          INT2FIX(2));
  rb_define_const(g_ktoblzcheck,  "BANK_NOT_KNOWN", INT2FIX(3));

  rb_define_method(g_ktoblzcheck, "initialize",     init_anc,     -1);
  rb_define_method(g_ktoblzcheck, "check",          check_acn,    2);
  rb_define_method(g_ktoblzcheck, "num_records",    num_records,  0);
  rb_define_method(g_ktoblzcheck, "close",          close_anc,    0);
  rb_define_method(g_ktoblzcheck, "find",           find_info,    1);

  g_ktoblzcheck_iban = rb_define_class_under(g_ktoblzcheck, "IBAN", rb_cObject);

  rb_define_const(g_ktoblzcheck_iban,   "VERSION",            rb_str_new2(AccountNumberCheck_libraryVersion())); /* yup. */

  rb_define_const(g_ktoblzcheck_iban,   "OK",                 INT2FIX(0));
  rb_define_const(g_ktoblzcheck_iban,   "TOO_SHORT",          INT2FIX(1));
  rb_define_const(g_ktoblzcheck_iban,   "PREFIX_NOT_FOUND",   INT2FIX(2));
  rb_define_const(g_ktoblzcheck_iban,   "WRONG_LENGTH",       INT2FIX(3));
  rb_define_const(g_ktoblzcheck_iban,   "COUNTRY_NOT_FOUND",  INT2FIX(4));
  rb_define_const(g_ktoblzcheck_iban,   "WRONG_COUNTRY",      INT2FIX(5));
  rb_define_const(g_ktoblzcheck_iban,   "BAD_CHECKSUM",       INT2FIX(6));

  rb_define_method(g_ktoblzcheck_iban,  "initialize",         init_ic,  -1);
  rb_define_method(g_ktoblzcheck_iban,  "check",              check_ic, 2);
  rb_define_method(g_ktoblzcheck_iban,  "close",              close_ic, 0);
}
