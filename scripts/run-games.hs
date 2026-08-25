#!/usr/bin/env stack
-- stack --resolver lts-24.43 script

{-# LANGUAGE OverloadedStrings #-}
{-# LANGUAGE PackageImports #-}
{-# OPTIONS_GHC -Wno-name-shadowing #-}
{-# OPTIONS_GHC -Wno-unused-do-bind #-}

module Main where

import Control.Exception (SomeException, finally, try)
import Control.Foldl qualified as Fold
import Control.Monad
import Data.Char
import Data.Either
import Data.Maybe
import System.Directory
import System.FilePath
import Turtle
import "text" Data.Text qualified as T
import "text" Data.Text.IO qualified as T
import Prelude

prependHome x = liftIO getHomeDirectory <&> (</> x)

cac = ".local/lutris-cache"

getstr s = has $ between s "\n" $ star (noneOf "\n")

metadata :: (MonadIO io) => FilePath -> io (Line, [Text])
metadata x = do
  fs <- liftIO $ T.readFile x
  pure (fromJust . textToLine . head $ match (getstr "Name=") fs, T.words $ head $ match (getstr "Exec=") fs)

steam :: Shell FilePath
steam = do
  f <- ls =<< prependHome ".local/share/applications"
  guard =<< liftIO (doesFileExist f)
  guard . not . null . match (has "Exec=steam ") . lineToText =<< input f
  pure f

wordedBy sep join f = T.intercalate join . map f . T.splitOn sep

modifyHead f t =
  case T.uncons t of
    Nothing -> t
    Just (c, cs) -> T.cons (f c) cs

unfart = wordedBy "-" " " $ modifyHead toUpper

trim :: Pattern Text
trim = has $ do "|"; spaces; s <- star (noneOf " "); spaces; "|"; pure s

things :: Shell Line
things = do
  ln <- lineToText <$> inshell "lutris -l 2>/dev/null" empty
  select $ textToLines $ match trim ln !! 1

recac = do
  c <- prependHome cac
  output c things

liftLine f = fromJust . textToLine . f . lineToText

luttyfart :: Line -> (Line, [Text])
luttyfart x = (liftLine unfart x, ["lutris", lineToText x])

main :: IO ()
main = flip finally recac $ sh $ do
  let xs = (luttyfart <$> (input =<< prependHome cac)) <|> (steam >>= metadata)
  str <- inshell "fuzzel --dmenu" (fst <$> xs)
  Just (c:as) <- fold xs (Fold.lookup str)
  proc c as empty
